
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
#include "objects/nftable.h"
#include "objects/nftab.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"

#include "vw_s1_vca_setup_main.h"
#include "vw_s1_vca_internal.h"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_sched_fixed = 0;
static NFOBJECT *g_all_obj = 0;
static NFOBJECT *g_tile_obj = 0;

static VCASchedData *g_sched_data = 0;


static void _draw_tile_area(NFTILE *tile, gint expose)
{
	gint i;
	guint color_n = 1;

	if (nfui_tile_drawable(NF_TILE(tile))) 
	{
		for (i = 0; i < 7*24; i++) 
		{
			switch(g_sched_data->sched[i]) 
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

static gboolean _get_mark_all_sched()
{
	gint i;
	gboolean status = TRUE;

	for (i = 0; i < 24*7; i++)
	{
		if (g_sched_data->sched[i] == '0') 
		{
			status = FALSE;
			break;		
		}
	}

	return status;
}

static gboolean post_all_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{	
		gint i;
		gboolean act = FALSE;
		gchar check;

		act = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		if (act) check = '1';
		else check = '0';

		for (i = 0; i < 24*7; i++)
		{
			g_sched_data->sched[i] = check;
		}

		_draw_tile_area(NF_TILE(g_tile_obj), 1);
	}

	return FALSE;
}

static gboolean tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint s_row = 0, s_col = 0;
	guint e_row = 0, e_col = 0;
	guint i, j;
	guint color_n = 0;
	guint mode;
	gboolean all_status;

	switch(evt->type) 
	{
		case NFEVENT_TILE_INIT:
		{
			_draw_tile_area(NF_TILE(obj), 1);
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

			gdk_display_get_pointer(gdk_display_get_default(), NULL, &mx, &my, NULL);
			mode = VW_RecSched_Control_Page(g_curwnd, x, y);

			if (mode == 2)
			{
				_draw_tile_area(NF_TILE(obj), 1);
			}
			else
			{
				switch (mode) 
				{
					case 0: color_n = SELECT_STATE_COLOR_2; break;
					case 1: color_n = SELECT_STATE_COLOR_3; break;
					default: break;
				}

				for (i = s_row; i <= e_row; i++) 
				{
					for(j = s_col; j <= e_col; j++) 
					{
						if (mode == 0) 		g_sched_data->sched[i*24+j] = '0';
						else if (mode == 1) g_sched_data->sched[i*24+j] = '1';

						nfui_tile_no_draw_color(NF_TILE(obj), color_n, i, j, i, j);
					}
				}

				_draw_tile_area(NF_TILE(obj), 1);

				all_status = _get_mark_all_sched();
				nfui_check_button_set_active((NFCHECKBUTTON*)g_all_obj, all_status);
				nfui_signal_emit(g_all_obj, GDK_EXPOSE, TRUE);
			}
		}
		break;

		default:
		break;
	}

	return FALSE;
}

static gboolean post_sched_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkColor off_color = UX_COLOR(1121);	
	GdkColor on_color = UX_COLOR(1122);	
	guint x, y;
	gint width, height;

	if (evt->type == GDK_EXPOSE) 
	{	
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &x, &y);
		nfui_nfobject_get_size(obj, &width, &height);
		
		gdk_gc_set_rgb_fg_color(gc, &off_color);					// OFF
		gdk_draw_rectangle(drawable, gc, TRUE, x+width-244, y, 22, 22);
		gdk_gc_set_rgb_fg_color(gc, &on_color);						// ON
		gdk_draw_rectangle(drawable, gc, TRUE, x+width-122, y, 22, 22);

		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)  
	{
		g_curwnd = 0;
		g_sched_fixed = 0;
	}

	return FALSE;
}

NFOBJECT* _S1_VCA_RuleSchedule_Page(NFOBJECT *parent, VCASchedData *data, gint page_x, gint page_y, gint page_w, gint page_h)
{
	NFOBJECT *fixed;
	NFOBJECT *fixed_temp;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	GdkColor ttile_color[4] = {UX_COLOR(1124),	
								UX_COLOR(1121),	
								UX_COLOR(1122),	
								UX_COLOR(TODO_COLOR)};

	const gchar *days[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
	gchar strBuf[16];

	gint pos_x, pos_y;
	guint chk_w, chk_h;
	gint i;
	gboolean all_status;

	g_curwnd = nfui_nfobject_get_top(parent);
	g_sched_data = data;
	
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(fixed, page_w, page_h);
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)parent, fixed, page_x, page_y);
	nfui_regi_post_event_callback(fixed, post_sched_fixed_event_handler);	
	g_sched_fixed = fixed;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OFF", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(1125));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_set_size(obj, 100, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, page_w-222, 0);	

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ON", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(1125));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_set_size(obj, 100, 22);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, page_w-100, 0);		

	pos_x = 0;
	pos_y = 24;

	fixed_temp = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(1126));
	nfui_nfobject_set_size(fixed_temp, 126, 264);
	nfui_nfobject_show(fixed_temp);
	nfui_nffixed_put((NFFIXED*)fixed, fixed_temp, pos_x, pos_y);

	all_status = _get_mark_all_sched();
	
	obj = nfui_checkbutton_new(all_status);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size(obj, &chk_w, &chk_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, 10, (264-chk_h)/2);	
	nfui_regi_post_event_callback(obj, post_all_check_event_handler);
	g_all_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_set_size(obj, 76, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1126));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed_temp, obj, 50, (264-40)/2);	

	pos_x += (126+1);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 116, 26);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	
	for (i = 0; i < 7; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(days[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 116, 33);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y + 27 + (34 * i));
	}

	pos_x += (116+2);

	for (i = 0; i < 24; i++)
	{
		sprintf(strBuf, "%02d", i);	
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 51, 26);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x + (52 * i), pos_y);
	}
	pos_y += 27;

	obj = nfui_tile_new(7, 24);
	nfui_tile_set_line_border(NF_TILE(obj), 1);	
	nfui_tile_set_color(NF_TILE(obj), NFTILE_STATE_FOCUS, &UX_COLOR(1123)); 
	nfui_tile_set_color(NF_TILE(obj), NFTILE_STATE_SELECT, ttile_color); 
	nfui_tile_set_drawable_outline(NF_TILE(obj), FALSE);
	nfui_nfobject_set_size(obj, 52*24, 34*7);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, tile_event_handler);
	g_tile_obj = obj;

	return g_sched_fixed;
}

gint _S1_VCA_RuleSchedule_set_channel(gint ch, VCASchedData *data)
{
	gboolean all_status;
	
	g_sched_data = data;

	all_status = _get_mark_all_sched();

	if (nfui_nfobject_is_shown(g_sched_fixed))
	{
		nfui_check_button_set_active((NFCHECKBUTTON*)g_all_obj, all_status);
		_draw_tile_area(NF_TILE(g_tile_obj), 1);
	}
	else
	{
		nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_all_obj, all_status);
		_draw_tile_area(NF_TILE(g_tile_obj), 0);
	}
	
	return 0;
}

