#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nftile.h"
#include "objects/nfspinbutton.h"
#include "objects/nfimage.h"

#include "vw_evt_act_main.h"
#include "vw_alarm_out_evt.h"
#include "vw_alarm_out_sched.h"
#include "vw_alarm_sched_ctrl.h"
#include "vw_copy_sched.h"
#include "log.h"
#include "scm.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (CAM_ALARM_OUT / PAGE_FIXED_CNT)

enum {
	SUNDAY = 0,
	MONDAY,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY,
	HOLIDAY,
	DAY
};

static EA_AlmSchedData	g_asd[CAM_ALARM_OUT];
static EA_AlmSchedData	g_oasd[CAM_ALARM_OUT];


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_dayObj;
static NFOBJECT *g_tileObj[ROW_CNT_PER_PAGE];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;




static void init_as_data();
static void set_as_data();
static void set_data_to_obj(gint expose);


static void init_as_data()
{
	guint i;

	memset(g_asd, 0x00, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT);
	memset(g_oasd, 0x00, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT);
	for(i=0; i<CAM_ALARM_OUT; i++)
		DAL_get_almSched_data(&g_asd[i], i);

	g_memmove(g_oasd, g_asd, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT);
}

static void set_as_data()
{
	DAL_set_almSched_data_all(g_asd, CAM_ALARM_OUT);

	scm_put_log(CHANGE_EVT_ALARMOUT, 0, 0);
}

static void set_data_to_obj(gint expose)
{
	gint day = -1;
	gint color_n = 0;
	gint i, j;
	gint page_num, row_num;

	day = nfui_spin_button_get_index(NF_SPINBUTTON(g_dayObj));

	if(day < 0) return;
	
    page_num = row_num = 0;

	for(i = 0; i < CAM_ALARM_OUT; i++) 
	{
		for(j = 0; j < 24; j++) 
		{									// time
			switch(g_asd[i].sched[day][j]) {
				case '0':
					color_n = SELECT_STATE_COLOR_1;
					break;

				case '1':
					color_n = SELECT_STATE_COLOR_2;
					break;

				case '2':
					color_n = SELECT_STATE_COLOR_3;
					break;

				default:
					color_n = SELECT_STATE_COLOR_1;
					break;
			}

			if (expose) {
			    if (nfui_nfobject_is_shown(g_page_fixed[page_num]))
    			    nfui_tile_draw_color(NF_TILE(g_tileObj[page_num]), (guint)color_n, row_num, j, row_num, j);
    			else
    			    nfui_tile_no_draw_color(NF_TILE(g_tileObj[page_num]), (guint)color_n, row_num, j, row_num, j);
		    }
			else {
			    nfui_tile_no_draw_color(NF_TILE(g_tileObj[page_num]), (guint)color_n, row_num, j, row_num, j);
		    }
		}

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
	}
}

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

static gboolean post_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) {
		set_data_to_obj(1);
	}

	return FALSE;
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFEVENT_TILE_INIT:
			{
				set_data_to_obj(1);
			}
			break;

		case NFEVENT_TILE_START_SELECT:
			{
				NF_TILE(obj)->select_n = SELECT_STATE_COLOR_1;
			}
			break;

		case NFEVENT_TILE_MOVE_SELECT:
			{
			}
			break;

		case NFEVENT_TILE_END_SELECT:
			{
				guint s_row = 0, s_col = 0;
				guint e_row = 0, e_col = 0;
				gint sched = -1;
				gint day = -1;
				gint i, j;
				gint x, y;
				gint mx, my;
				int off_x, off_y;
                gint tile_idx, ch;

				nfui_tile_get_selectArea(NF_TILE(obj), &s_row, &s_col, &e_row, &e_col);
				x = (50 * (e_col + 1));
				y = (42 * (e_row + 1)); 

				nfui_nfobject_get_offset(obj, &off_x, &off_y);
				x += off_x;
				y += off_y;

				gdk_display_get_pointer(gdk_display_get_default(),
									NULL, &mx, &my, NULL);

				if (mx > x - 50 && mx < x + 50) x = mx;
				if (my > y - 42 && my < y + 42) y = my;
					 
				sched = VW_AlmSched_Ctrl(g_curwnd, x, y);

				if (sched == -1) {
					set_data_to_obj(1);
					return FALSE;
				}

				day = nfui_spin_button_get_index(NF_SPINBUTTON(g_dayObj));
				if(day < 0)
					return FALSE;

				tile_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "tile index"));	

				for(i=s_row; i<=e_row; i++) 
				{							
                    ch = i + (ROW_CNT_PER_PAGE * tile_idx);

					for(j=s_col; j<=e_col; j++) 
					{						
						if(sched < 0) 
						{
							switch(g_asd[ch].sched[day][j]) 
							{
								case '0':
									nfui_tile_draw_color(NF_TILE(g_tileObj[tile_idx]), SELECT_STATE_COLOR_1, i, j, i, j);
									break;

								case '1':
									nfui_tile_draw_color(NF_TILE(g_tileObj[tile_idx]), SELECT_STATE_COLOR_2, i, j, i, j);
									break;

								case '2':
									nfui_tile_draw_color(NF_TILE(g_tileObj[tile_idx]), SELECT_STATE_COLOR_3, i, j, i, j);
									break;

								default:
									nfui_tile_draw_color(NF_TILE(g_tileObj[tile_idx]), SELECT_STATE_COLOR_1, i, j, i, j);
									break;
							}
						}
						else 
						{
							if(sched == 0) 		g_asd[ch].sched[day][j] = '0';
							else if(sched == 1) g_asd[ch].sched[day][j] = '1';
							else if(sched == 2) g_asd[ch].sched[day][j] = '2';

							nfui_tile_draw_color(NF_TILE(g_tileObj[tile_idx]), (guint)SELECT_STATE_COLOR_1 + sched, i, j, i, j);
						}
					}
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_copy_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint day;
		guint day_mask = 0;
		gint i, j;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		day = nfui_spin_button_get_index(NF_SPINBUTTON(g_dayObj));

		day_mask = VW_Copy_Sched(g_curwnd, (guint)day);

		for(i=0; i<CAM_ALARM_OUT; i++) {
			for(j=0; j<7; j++) {
				if(j == day) continue;

				if(day_mask & (1 << j)) 
					memcpy(g_asd[i].sched[j], g_asd[i].sched[day], 24);

				// for debug
				//g_asd[ch1].sched[mon], g_asd[ch1].sched[mon];
				//g_asd[ch1].sched[tue], g_asd[ch1].sched[mon];
				//g_asd[ch1].sched[wed], g_asd[ch1].sched[mon];
				//g_asd[ch1].sched[tur], g_asd[ch1].sched[mon];
				//g_asd[ch1].sched[fri], g_asd[ch1].sched[mon];
				//g_asd[ch1].sched[sat], g_asd[ch1].sched[mon];
				//g_asd[ch1].sched[sun], g_asd[ch1].sched[mon];
			}
		}
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

			g_memmove(g_asd, g_oasd, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT);

			set_data_to_obj(1);
	}
	
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if(memcmp(g_oasd, g_asd, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT)) {
			set_as_data();

			g_memmove(g_oasd, g_asd, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			event_act_data_changed(TRUE);
		}
	}
	
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		VW_AlarmOut_tab_out_handler();
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

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
				off_x + 1014, 
				off_y + 22,
				17, 17);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(780));	// ON
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1185, 
				off_y + 22,
				17, 17);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(781));	// EVENT
		gdk_draw_rectangle(drawable, 
				gc, TRUE, 
				off_x + 1355, 
				off_y + 22,
				17, 17);

		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

void VW_Init_AlarmOut_SchedPage(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *tile;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

	gchar *day[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	gchar *legend[] = {"OFF", "ON", "EVENT"};
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
						IMG_CAMERA_OUTPUT_ICON_32};

	GdkColor tile_color[4] = {UX_COLOR(779), UX_COLOR(780),	UX_COLOR(781), UX_COLOR(779)};
	gchar strBuf[32];
	gint i;
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];


	g_curwnd = nfui_nfobject_get_top(parent);


	// init data
	init_as_data();
	
	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	for(i=0; i<3; i++) {
		if(nftool_cur_language_is_japanese())
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(legend[i], nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(968));
		else			
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(legend[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 130, 17);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 1042 + (i * (155 + 17)), 24);
	}

	// spin
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("YOIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 140, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 4, 0);

	g_dayObj = nfui_spinbutton_new(day, 7, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_dayObj, NFSPINBUTTON_TYPE_SUBTAB_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)g_dayObj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_dayObj, 154, 40);
	nfui_regi_post_event_callback(g_dayObj, post_spin_event_handler);
	nfui_nfobject_show(g_dayObj);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_dayObj, 143, 0);



	for (i = 0; i < 24; i++) {
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
	for (i = 0; i < CAM_ALARM_OUT; i++) 
	{
		sprintf(strBuf, "AO %d", i + 1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_set_size(obj, 236 + 58, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 6);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], obj, 0, (row_num * 42));

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
    	nfui_tile_set_color(NF_TILE(g_tileObj[i]), NFTILE_STATE_FOCUS, &UX_COLOR(782)); 
    	nfui_tile_set_color(NF_TILE(g_tileObj[i]), NFTILE_STATE_SELECT, tile_color); 
    	nfui_nfobject_set_size(g_tileObj[i], (50 * 24), (42 * ROW_CNT_PER_PAGE));
    	nfui_nfobject_show(g_tileObj[i]);
    	nfui_regi_post_event_callback(g_tileObj[i], post_tile_event_handler);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[i], g_tileObj[i], 297, 0);

    	nfui_nfobject_set_data(g_tileObj[i], "tile index", GINT_TO_POINTER(i));
	}

	/* button */
	obj = nftool_normal_button_create_subtab_type1("COPY SCHEDULE TO", 224);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, (MENU_V_SUBTAB_INNER_W-226-7), (MENU_V_SUBTAB_INNER_H-40));
	nfui_regi_post_event_callback(obj, post_copy_button_event_handler);


	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
	nfui_regi_post_event_callback(content_fixed, post_content_fixed_event_handler);	
}

gboolean check_alarm_sched_data_changed()
{
	if(!memcmp(g_oasd, g_asd, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT))
		return FALSE;

	return TRUE;
}

void save_alarm_sched_data()
{
	g_memmove(g_oasd, g_asd, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT);

	set_as_data();
}

void restroe_alarm_sched_data()
{
	g_memmove(g_asd, g_oasd, sizeof(EA_AlmSchedData) * CAM_ALARM_OUT);

	set_data_to_obj(0);
}
