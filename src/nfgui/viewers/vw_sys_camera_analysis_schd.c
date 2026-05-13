/*
 * vw_sys_camera_analysis_schd.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 28, 2019
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
#include "objects/nfimglabel.h"
#include "objects/nftab.h"
#include "objects/nftile.h"
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_menu.h"
#include "vw_sys_camera_analysis_act.h"
#include "vw_sys_camera_analysis_prop.h"
#include "vw_sys_camera_analysis_schd.h"

#include "scm.h"

#include "nf_api_ipcam.h"
#include "nf_api_dlva.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)






////////////////////////////////////////////////////////////
//
// private data types
//




////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_cam_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_dayObj = 0;
static NFOBJECT *g_tileObj[ROW_CNT_PER_PAGE];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static AiAnalysisSchedData g_asd[GUI_CHANNEL_CNT];
static AiAnalysisSchedData g_org_asd[GUI_CHANNEL_CNT];

static gint g_day = 0;


////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _init_schedule(void)
{
	DAL_get_aianalysis_schd_data_all(g_asd, GUI_CHANNEL_CNT);
	memcpy(g_org_asd, g_asd, sizeof(AiAnalysisSchedData)*GUI_CHANNEL_CNT);

	return 0;
}

static gint _save_schedule(void)
{
    DAL_set_aianalysis_schd_data_all(g_asd, GUI_CHANNEL_CNT);
    memcpy(g_org_asd, g_asd, sizeof(AiAnalysisSchedData)*GUI_CHANNEL_CNT);

	scm_put_log(CHANGE_CAM_VCA, 0, 0);
	return 0;
}


static gint _set_schedule(gboolean expose)
{
	guint i, j;
	guint color;
	gint page_num, row_num;
	
    page_num = row_num = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		for (j = 0; j < 24; j++) {
			switch ( g_asd[i].sched[j+(g_day*24)] ) {
				case '1':
					color = SELECT_STATE_COLOR_2;
					break;
				case '0':
				default:
					color = SELECT_STATE_COLOR_1;
					break;
			}
			
			if (expose) {
			    if (nfui_nfobject_is_shown(g_page_fixed[page_num]))
    			    nfui_tile_draw_color(NF_TILE(g_tileObj[page_num]), (guint)color, row_num, j, row_num, j);
    			else
    			    nfui_tile_no_draw_color(NF_TILE(g_tileObj[page_num]), (guint)color, row_num, j, row_num, j);
		    }
			else {
			    nfui_tile_no_draw_color(NF_TILE(g_tileObj[page_num]), (guint)color, row_num, j, row_num, j);
		    }
		}

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}
	return 0;
}

static gint _draw_tile_area(NFTILE *tile, gint expose)
{
	gint i;
	guint color_n = 1;

	if (nfui_tile_drawable(NF_TILE(tile))) 
	{
		for (i = 0; i < 7*24; i++) 
		{
			switch(g_asd->sched[i]) 
			{
				case '0': color_n = SELECT_STATE_COLOR_2; break;
				case '1': color_n = SELECT_STATE_COLOR_3; break;
				default: break;
			}

			if (expose) nfui_tile_draw_color(NF_TILE(tile), color_n, i/24, i%24, i/24, i%24);
			else		nfui_tile_no_draw_color(NF_TILE(tile), color_n, i/24, i%24, i/24, i%24);			
		}
	}

	return 0;
}



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);
    	
        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint day;
	gint i;

	if (evt->type == NFEVENT_COMBOBOX_CHANGED ) 
	{
		g_day = (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);	
		_set_schedule(TRUE);
	}

    return FALSE;
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{
		case NFEVENT_TILE_INIT:
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
                gint tile_idx, ch;

				gdk_display_get_pointer(gdk_display_get_default(),
						NULL, &x, &y, NULL);
				x = MIN(x, 1920 - 220);
				y = MIN(y, 1080 - 150);

				sched = VW_RecSched_Control_Page(g_curwnd, (guint)x, (guint)y);

				nfui_tile_get_selectArea(NF_TILE(obj),
						&s_row, &s_col, &e_row, &e_col);

				tile_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "tile index"));

				for (i = s_row; i <= e_row; i++)
				{
					ch = i + (ROW_CNT_PER_PAGE * tile_idx);

					for (j = s_col; j <= e_col; j++)
					{
						if (sched == 2)
						{
							switch (g_asd[ch].sched[j + (g_day * 24)])
							{
							case '1':
								color = SELECT_STATE_COLOR_2;
								break;
							case '0':
							default:
								color = SELECT_STATE_COLOR_1;
								break;
							}
						}
						else
						{
							g_asd[ch].sched[j + (g_day * 24)] = (gchar)('0' + sched);
							color = SELECT_STATE_COLOR_1 + sched;
						}
						nfui_tile_draw_color(NF_TILE(g_tileObj[tile_idx]),
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

        if(memcmp(&g_asd, &g_org_asd, sizeof(AiAnalysisSchedData)*GUI_CHANNEL_CNT))
        {
            memcpy(g_asd, g_org_asd, sizeof(AiAnalysisSchedData)*GUI_CHANNEL_CNT);
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

        if(memcmp(&g_asd, &g_org_asd, sizeof(AiAnalysisSchedData)*GUI_CHANNEL_CNT))
        {
            _save_schedule();
        	nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			syscam_set_changeflag(1);
        }
	}

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_analysis_schd_tab_out_handler();
        SystemSetupCam_Destroy(obj);
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
				off_x + 1185, 
				off_y + 22,
				17, 17);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(780));	// ON
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1355, 
				off_y + 22,
				17, 17);

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

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
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

gint VW_analysis_schd_init_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *tile;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

	gchar *day[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	gchar *legend[] = {"OFF", "ON"};
    GdkPixbuf *pbCamImage[32];

	GdkColor tile_color[4] = {UX_COLOR(779), UX_COLOR(780), UX_COLOR(781), UX_COLOR(779)};
	gchar strBuf[32];
	gint i;
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];


    pbCamImage[0] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL);
    pbCamImage[1] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL);
    pbCamImage[2] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL);
    pbCamImage[3] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL);
    pbCamImage[4] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL);
    pbCamImage[5] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL);
    pbCamImage[6] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL);
    pbCamImage[7] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL);
    pbCamImage[8] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL);
    pbCamImage[9] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL);
    pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL);
    pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL);
    pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL);
    pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL);
    pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL);
    pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL);
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL);
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL);
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL);
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL);
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL);
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL);
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL);
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL);
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL);
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL);
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL);
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL);
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL);
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL);
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL);
	
	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

	g_day = 0;
	_init_schedule();


    g_curwnd = nfui_nfobject_get_top(parent);


    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_regi_pre_event_callback(content_fixed, post_content_fixed_event_handler);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);

    for(i = 0; i < 2; i++) 
	{
		if (nftool_cur_language_is_japanese()) obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(legend[i], nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(968));
		else obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(legend[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 130, 17);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 1042 + ((i+1) * (155 + 17)), 22);
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DAY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 140, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 4, 0);

    obj = nfui_combobox_new(day, 7, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_nfobject_set_size(obj, 154, 40);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, 143, 0);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_channel_event_handler);
    g_dayObj = obj;

	for (i = 0; i < 24; i++) 
	{
		sprintf(strBuf, "%02d", i);	
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_set_size(obj, 48, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 301 + (i * 50), 47);
	}
	
    size_w = 236 + 58 + (50 * 24);

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 75);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, 4, 87);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;
	
	memset(strBuf, 0x00, sizeof(strBuf));
	
	for (i = 0; i < GUI_CHANNEL_CNT; i++) 
	{
        memset(strBuf, 0x00, sizeof(strBuf));
        var_get_camtitle(strBuf, i);

        obj = (NFOBJECT *)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL *)obj, NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL *)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_set_size(obj, 236 + 58, 40);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], obj, 0, (row_num * 42));
		g_cam_obj[i] = obj;

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}

	// tile
	for (i = 0; i < PAGE_FIXED_CNT; i++)
	{
    	g_tileObj[i] = nfui_tile_new(ROW_CNT_PER_PAGE, 24);
		nfui_tile_set_color(NF_TILE(g_tileObj[i]), NFTILE_STATE_SELECT, tile_color); 
    	nfui_nfobject_set_size(g_tileObj[i], (50 * 24), (42 * ROW_CNT_PER_PAGE));
    	nfui_nfobject_show(g_tileObj[i]);
    	nfui_regi_post_event_callback(g_tileObj[i], post_tile_event_handler);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[i], g_tileObj[i], 297, 0);

    	nfui_nfobject_set_data(g_tileObj[i], "tile index", GINT_TO_POINTER(i));
	}

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

    return 0;
}

gboolean VW_analysis_schd_tab_in_handler()
{



    return FALSE;
}

gboolean VW_analysis_schd_tab_out_handler()
{
    mb_type ret;

    if(memcmp(&g_asd, &g_org_asd, sizeof(AiAnalysisSchedData)*GUI_CHANNEL_CNT))
    {
	    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
	    if(ret == NFTOOL_MB_OK) 
	    {
            _save_schedule();
        	syscam_set_changeflag(1);
        }
        else if(ret == NFTOOL_MB_CANCEL)
        {
            memcpy(g_asd, g_org_asd, sizeof(AiAnalysisSchedData)*GUI_CHANNEL_CNT);
            _set_schedule(TRUE);
        }
    }    
    
    return FALSE;
}
