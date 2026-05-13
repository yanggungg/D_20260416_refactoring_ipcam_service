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

#if defined(_IPX_1648P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
#define NVR_ALARM_OUT_COUNT	DVR_ALARM_OUT
#else
#define NVR_ALARM_OUT_COUNT	ALARM_OUT_COUNT
#endif

#if defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
#define RECORDER_ALARM_OUT_OFFSET	32
#else
#define RECORDER_ALARM_OUT_OFFSET	16
#endif

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

static EA_AlmSchedData	g_asd[NVR_ALARM_OUT_COUNT];
static EA_AlmSchedData	g_oasd[NVR_ALARM_OUT_COUNT];


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_dayObj;
static NFOBJECT *g_tileObj;




static void init_as_data();
static void set_as_data();
static void set_data_to_obj(gint expose);


static void init_as_data()
{
	guint i;

	memset(g_asd, 0x00, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT);
	memset(g_oasd, 0x00, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT);
	for(i=0; i<NVR_ALARM_OUT_COUNT; i++)
		DAL_get_almSched_data(&g_asd[i], i + RECORDER_ALARM_OUT_OFFSET);

	g_memmove(g_oasd, g_asd, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT);
}

static void set_as_data()
{
	guint i;

	for(i=0; i<NVR_ALARM_OUT_COUNT; i++)
		DAL_set_almSched_data(g_asd[i], i + RECORDER_ALARM_OUT_OFFSET);
	//DAL_set_almSched_data_all(g_asd, NVR_ALARM_OUT_COUNT);

	scm_put_log(CHANGE_EVT_ALARMOUT, 0, 0);
}

static void set_data_to_obj(gint expose)
{
	gint day = -1;
	gint color_n = 0;
	gint i, j;

	day = nfui_spin_button_get_index(NF_SPINBUTTON(g_dayObj));

	if(day < 0) return;

	for(i=0; i<NVR_ALARM_OUT_COUNT; i++) {
		for(j=0; j<24; j++) {									// time
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

			if (expose) nfui_tile_draw_color(NF_TILE(g_tileObj), (guint)color_n, i, j, i, j);
			else nfui_tile_no_draw_color(NF_TILE(g_tileObj), (guint)color_n, i, j, i, j);
		}
	}
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

				for(i=s_row; i<=e_row; i++) {							// ch
					for(j=s_col; j<=e_col; j++) {						// time
						if(sched < 0) 
						{
							switch(g_asd[i].sched[day][j]) 
							{
								case '0':
									nfui_tile_draw_color(NF_TILE(g_tileObj), SELECT_STATE_COLOR_1, i, j, i, j);
									break;

								case '1':
									nfui_tile_draw_color(NF_TILE(g_tileObj), SELECT_STATE_COLOR_2, i, j, i, j);
									break;

								case '2':
									nfui_tile_draw_color(NF_TILE(g_tileObj), SELECT_STATE_COLOR_3, i, j, i, j);
									break;

								default:
									nfui_tile_draw_color(NF_TILE(g_tileObj), SELECT_STATE_COLOR_1, i, j, i, j);
									break;
							}
						}
						else 
						{
							if(sched == 0) 		g_asd[i].sched[day][j] = '0';
							else if(sched == 1) g_asd[i].sched[day][j] = '1';
							else if(sched == 2) g_asd[i].sched[day][j] = '2';

							nfui_tile_draw_color(NF_TILE(g_tileObj), (guint)SELECT_STATE_COLOR_1 + sched, i, j, i, j);
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

		for(i=0; i<NVR_ALARM_OUT_COUNT; i++) {
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

			g_memmove(g_asd, g_oasd, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT);

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

		if(memcmp(g_oasd, g_asd, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT)) {
			set_as_data();

			g_memmove(g_oasd, g_asd, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT);

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

void VW_Nvr_Init_AlarmOut_SchedPage(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *tile;

	gchar *day[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	gchar *legend[] = {"OFF", "ON", "EVENT"};

	GdkColor tile_color[4] = {UX_COLOR(779), UX_COLOR(780),	UX_COLOR(781), UX_COLOR(779)};
	gchar strBuf[32];
	gint i;


	g_curwnd = nfui_nfobject_get_top(parent);


	// init data
	init_as_data();

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

	for (i = 0; i < NVR_ALARM_OUT_COUNT; i++) 
	{
    	memset(strBuf, 0x00, sizeof(strBuf));
		if(NVR_RELAY_OUT > i)  g_sprintf(strBuf, "R%d", i + 1);
		else 					sprintf(strBuf, "AO %d", (i - NVR_RELAY_OUT) + 1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_set_size(obj, 236 + 58, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 6);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 4, 87 + (i * 42));
	}


	// tile
	g_tileObj = nfui_tile_new(NVR_ALARM_OUT_COUNT, 24);
	nfui_tile_set_color(NF_TILE(g_tileObj), NFTILE_STATE_FOCUS, &UX_COLOR(782)); 
	nfui_tile_set_color(NF_TILE(g_tileObj), NFTILE_STATE_SELECT, tile_color); 
	nfui_nfobject_set_size(g_tileObj, (50 * 24), (42 * NVR_ALARM_OUT_COUNT));
	nfui_nfobject_show(g_tileObj);
	nfui_regi_post_event_callback(g_tileObj, post_tile_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_tileObj, 301, 87);



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

gboolean check_nvr_alarm_sched_data_changed()
{
	if(!memcmp(g_oasd, g_asd, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT))
		return FALSE;

	return TRUE;
}

void save_nvr_alarm_sched_data()
{
	g_memmove(g_oasd, g_asd, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT);

	set_as_data();
}

void restroe_nvr_alarm_sched_data()
{
	g_memmove(g_asd, g_oasd, sizeof(EA_AlmSchedData) * NVR_ALARM_OUT_COUNT);

	set_data_to_obj(0);
}
