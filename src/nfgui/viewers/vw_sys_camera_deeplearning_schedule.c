#include <string.h>
#include "nf_afx.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_camera_deeplearning.h"
#include "vw_sys_camera_deeplearning_property.h"
#include "vw_sys_camera_deeplearning_schedule.h"
#include "vw_record_sched_control.h"

#include "nf_notify.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nftile.h"

#include "vw_vkeyboard.h"
#include "nfdal.h"
#include "scm.h"



////////////////////////////////////////////////////////////
//
// private data types
//

enum {
	SUNDAY = 0,
	MONDAY,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY,
	DAY
};



////////////////////////////////////////////////////////////
//
// private variable
//

static DVASchedData g_sched_data[GUI_CHANNEL_CNT];
static DVASchedData g_org_sched_data[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_cam_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_dayObj = 0;
static NFOBJECT *g_tileObj = 0;





////////////////////////////////////////////////////////////
//
// private interfaces 
//

static void redraw_tile_modeArea(NFTILE *tile, guint day, gint expose)
{
    guint i, j;
    guint color_n = 1;

	for (i = 0; i < GUI_CHANNEL_CNT; i++) 
	{
		for(j = 0; j < 24; j++) 
		{
			if (g_sched_data[i].sched[day][j] == '0') color_n = SELECT_STATE_COLOR_2;
			else if (g_sched_data[i].sched[day][j] == '1') color_n = SELECT_STATE_COLOR_3;

			if (expose) nfui_tile_draw_color(NF_TILE(tile), color_n, i, j, i, j);
			else nfui_tile_no_draw_color(NF_TILE(tile), color_n, i, j, i, j);
		}
	}
}



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_day_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint day;

		day = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		redraw_tile_modeArea((NFTILE*)g_tileObj, day, 1);
	}

	return FALSE;
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint s_row = 0, s_col = 0;
    guint e_row = 0, e_col = 0;
    guint i, j;
    guint sched;
	gint day;

    switch(evt->type) 
	{
        case NFEVENT_TILE_INIT:
		{
			day = nfui_combobox_get_cur_index(NF_COMBOBOX(g_dayObj));
			redraw_tile_modeArea(NF_TILE(obj), day, 1);
        }       
        break;
        
        case NFEVENT_TILE_START_SELECT:
            NF_TILE(obj)->select_n = SELECT_STATE_COLOR_1;
        break;
                        
        case NFEVENT_TILE_MOVE_SELECT:
        break;

        case NFEVENT_TILE_END_SELECT:
        {
            gint x, y;
            gint mx, my;
            int off_x, off_y;

            nfui_tile_get_selectArea(NF_TILE(obj), &s_row, &s_col, &e_row, &e_col);
            x = (50 * (e_col + 1));
            y = (41 * (e_row + 1)); 

            nfui_nfobject_get_offset(obj, &off_x, &off_y);
            x += off_x;
            y += off_y;

            gdk_display_get_pointer(gdk_display_get_default(), NULL, &mx, &my, NULL);

            if (mx > x - 50 && mx < x + 50) x = mx;
            if (my > y - 41 && my < y + 41) y = my;

            sched = VW_RecSched_Control_Page(g_curwnd, x, y);

            if (sched == 2)
            {
				day = nfui_combobox_get_cur_index(NF_COMBOBOX(g_dayObj));
                redraw_tile_modeArea(NF_TILE(obj), day, 1);
            }
            else
            {
				day = nfui_combobox_get_cur_index(NF_COMBOBOX(g_dayObj));

                for (i = s_row; i <= e_row; i++) 
				{
                    for(j = s_col; j <= e_col; j++) 
					{
						if (sched == 0) g_sched_data[i].sched[day][j] = '0';
						else if (sched == 1) g_sched_data[i].sched[day][j] = '1';
                    }
                }
                redraw_tile_modeArea(NF_TILE(obj), day, 1);
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
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
		gint day;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_memmove(g_sched_data, g_org_sched_data, sizeof(DVASchedData) * GUI_CHANNEL_CNT);

		day = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		redraw_tile_modeArea((NFTILE*)g_tileObj, day, 1);
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
		if(memcmp(g_org_sched_data, g_sched_data, sizeof(DVASchedData) * GUI_CHANNEL_CNT)) 
		{
			DAL_set_dva_schd_data_all(g_sched_data, GUI_CHANNEL_CNT);
			g_memmove(g_org_sched_data, g_sched_data, sizeof(DVASchedData) * GUI_CHANNEL_CNT);
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			syscam_set_changeflag(1);
		}		
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        CamDeepLearning_Schedule_tab_out_handler();
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

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(800));
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1185, 
				off_y + 22,
				17, 17);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(801));
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1355, 
				off_y + 22,
				17, 17);

		nfui_nfobject_gc_unref(gc);
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

void init_CamDeepLearning_Schedule_Page(NFOBJECT *parent)
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

	GdkColor tile_color[4] = {UX_COLOR(TODO_COLOR), UX_COLOR(800), UX_COLOR(801), UX_COLOR(TODO_COLOR)};
	gchar strBuf[32];
	gint i;

	gint scrolledfixed_h;


	memset(g_sched_data, 0x00, sizeof(DVASchedData) * GUI_CHANNEL_CNT);
	memset(g_org_sched_data, 0x00, sizeof(DVASchedData) * GUI_CHANNEL_CNT);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
		DAL_get_dva_schd_data(&g_sched_data[i], i);

	g_memmove(g_org_sched_data, g_sched_data, sizeof(DVASchedData) * GUI_CHANNEL_CNT);


	g_curwnd = nfui_nfobject_get_top(parent);


    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
    nfui_regi_pre_event_callback(content_fixed, post_content_fixed_event_handler);

	for (i = 0; i < 2; i++) 
    {
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

    obj = nfui_combobox_new(day, 7, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_nfobject_set_size(obj, 154, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, 143, 0);
    nfui_regi_post_event_callback(obj, post_day_combo_event_handler);
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

	g_tileObj = nfui_tile_new(GUI_CHANNEL_CNT, 24);
	nfui_tile_set_color(NF_TILE(g_tileObj), NFTILE_STATE_FOCUS, &UX_COLOR(802)); 
	nfui_tile_set_color(NF_TILE(g_tileObj), NFTILE_STATE_SELECT, tile_color); 
	nfui_nfobject_set_size(g_tileObj, (46 * 24), (42 * GUI_CHANNEL_CNT));
	nfui_nfobject_show(g_tileObj);
	nfui_regi_post_event_callback(g_tileObj, post_tile_event_handler);
	nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, g_tileObj, 297, 0);



    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
    
    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean CamDeepLearning_Schedule_tab_in_handler()
{

    return FALSE;
}

gboolean CamDeepLearning_Schedule_tab_out_handler()
{
	mb_type ret;
	gint day;

	day = nfui_combobox_get_cur_index(NF_COMBOBOX(g_dayObj));

	if (!memcmp(g_org_sched_data, g_sched_data, sizeof(DVASchedData)*GUI_CHANNEL_CNT)) return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{
		g_memmove(g_org_sched_data, g_sched_data, sizeof(DVASchedData)*GUI_CHANNEL_CNT);
		DAL_set_dva_prop_data_all(g_sched_data, GUI_CHANNEL_CNT);
		syscam_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(g_sched_data, g_org_sched_data, sizeof(DVASchedData)*GUI_CHANNEL_CNT);
		redraw_tile_modeArea((NFTILE*)g_tileObj, day, 0);
	}	

    return FALSE;
}
