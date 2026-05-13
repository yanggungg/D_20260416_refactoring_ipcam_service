
#include "ix_mem.h"
#include "ix_conf.h"

#include "../../support/util.h"
#include "../../support/event_loop.h"
#include "../../support/color.h"
#include "../../tools/nf_ui_tool.h"

#include "nfdrawarea.h"


#define		MAX_AREA_COUNT			(1)

// TODO: temporary min size
#define		MIN_AREA_SIZE_W				(30)
#define		MIN_AREA_SIZE_H				(30)

#define 	LINE_WIDTH				(4)

#define 	OUTRECT_SIZE			(10)
#define 	OUTRECT_GAP				((LINE_WIDTH/2)+OUTRECT_SIZE)


// TODO:..
#if 0
static void draw_polygon(NFOBJECT *obj, GdkWindow *wnd, gint x, gint y)
{
	GdkRectangle update_rec;
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *gc;
	static gint pre_x = -1, pre_y = -1;

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(gc, &colors[NF_COLOR_CODE_FFFFFF]);

#if 0
	update_rec.x = x - 3;
	update_rec.y = y - 3;
	update_rec.width = 6;
	update_rec.height = 6;

	gdk_draw_rectangle(drawable, gc, TRUE, update_rec.x, update_rec.y, update_rec.width, update_rec.height);
	gdk_window_invalidate_rect(wnd, &update_rec, FALSE);
#endif

	if(pre_x < 0 && pre_y < 0) 
	{
		pre_x = x;
		pre_y = y;

		return;
	}

	gdk_draw_line(drawable, gc, pre_x, pre_y, x, y);

	pre_x = x;
	pre_y = y;

	nfui_nfobject_gc_unref(gc);
}
#endif

static void sort_points(GdkPoint *to, GdkPoint *from)
{
	if((from[0].x < from[2].x) && (from[0].y < from[2].y)) 
	{
		memcpy(to, from, sizeof(GdkPoint) * MAX_POINTS);
	}
	else if((from[0].x > from[2].x) && (from[0].y < from[2].y)) 
	{
		to[0].x = from[1].x; to[0].y = from[1].y;
		to[1].x = from[0].x; to[1].y = from[0].y;
		to[2].x = from[3].x; to[2].y = from[3].y;
		to[3].x = from[2].x; to[3].y = from[2].y;
		to[4].x = from[1].x; to[4].y = from[1].y;
	}
	else if((from[0].x < from[2].x) && (from[0].y > from[2].y)) 
	{
		to[0].x = from[3].x; to[0].y = from[3].y;
		to[1].x = from[2].x; to[1].y = from[2].y;
		to[2].x = from[1].x; to[2].y = from[1].y;
		to[3].x = from[0].x; to[3].y = from[0].y;
		to[4].x = from[3].x; to[4].y = from[3].y;
	}
	else if((from[0].x > from[2].x) && (from[0].y > from[2].y)) 
	{
		to[0].x = from[2].x; to[0].y = from[2].y;
		to[1].x = from[3].x; to[1].y = from[3].y;
		to[2].x = from[0].x; to[2].y = from[0].y;
		to[3].x = from[1].x; to[3].y = from[1].y;
		to[4].x = from[2].x; to[4].y = from[2].y;
	}
}

static gboolean check_coord_in_rect(GdkRectangle rect, gint x, gint y)
{
	if(rect.x <= x && (rect.x + rect.width) >= x) 
	{
		if(rect.y <= y && (rect.y + rect.height) >= y) 
			return TRUE;
	}

	return FALSE;
}

static gboolean check_area_size_limit(NFOBJECT *obj, GdkPoint *points)
{
	gint w = (points[2].x - points[0].x);
	gint h = (points[2].y - points[0].y);
	
	if(w < 0) w *= -1;
	if(h < 0) h *= -1;

	if(w <= obj->width && h <= obj->height 
			&& w >= MIN_AREA_SIZE_W && h >= MIN_AREA_SIZE_H)
		return TRUE;

	return FALSE;
}

static gboolean set_points(NFOBJECT *obj, gint step, gint x, gint y)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;

	switch(step) 
	{
		case DRAW_START_STEP:
			{
				if(darea->points[0].x != -1 && darea->points[0].y != -1)
					return FALSE;

				darea->points[0].x = x;
				darea->points[0].y = y;
			}
			break;

		case DRAW_EDIT_STEP:
			{
				gint sx, sy;
				gint x_gap, y_gap;
	
				if(darea->points[0].x < 0 && darea->points[0].y < 0)
					return FALSE;

				sx = sy = x_gap = y_gap = 0;

				sx = darea->points[0].x;
				sy = darea->points[0].y;

				x_gap = (x - sx);
				y_gap = (y - sy);

				darea->points[1].x = sx + x_gap; 
				darea->points[1].y = sy;
				darea->points[2].x = x;
				darea->points[2].y = y;
				darea->points[3].x = sx;
				darea->points[3].y = sy + y_gap;
				darea->points[4].x = sx;
				darea->points[4].y = sy;
			}
			break;

		case DRAW_MODIFY_STEP:
			{
				GdkPoint *ppoint;
				GdkRectangle rect;
				gint gap_x = -1, gap_y = -1;
				gint idx = -1;
				gint i;

				for(i=0; i<(gint)darea->list_length; i++) {
					ppoint = (GdkPoint*)g_slist_nth_data(darea->list, (guint)i);

					rect.x = ppoint[0].x;
					rect.y = ppoint[0].y;
					rect.width = ppoint[2].x - ppoint[0].x;
					rect.height = ppoint[2].y - ppoint[0].y;

					if(check_coord_in_rect(rect, x, y)) 
					{
						if(gap_x < 0 && gap_y < 0) 
						{
							gap_x = rect.x - x;
							gap_y = rect.y - y;

							idx = i;
						}
						else if(gap_x > (rect.x - x) && gap_y > (rect.y - y)) 
						{
							gap_x = rect.x - x;
							gap_y = rect.y - y;

							idx = i;
						}
					}
				}

				if(idx < 0) 
				{
					memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
					return FALSE;
				}
				else 
				{
					darea->focus_area = idx;
					darea->area_state = FOCUS_STATE;

					ppoint = (GdkPoint*)g_slist_nth_data(darea->list, (guint)idx);
					memcpy(darea->points, ppoint, sizeof(GdkPoint) * MAX_POINTS);
				}
			}
			break;

		case DRAW_END_STEP:
			{
				GdkPoint *ppoint;
				gint i;

				if(darea->points[4].x < 0 && darea->points[4].y < 0) 
				{
					memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
					return FALSE;
				}

				if(darea->step == DRAW_MODIFY_STEP) 
				{
					if(darea->focus_area >= 0) 
					{
						ppoint = (GdkPoint*)g_slist_nth_data(darea->list, (guint)darea->focus_area);
						sort_points(ppoint, darea->points);

						darea->focus_area = -1;
						darea->area_state = NORMAL_STATE;
						darea->resize_side = NONE_SIDE;

						memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
					}
				}
				else 
				{
					if(!check_area_size_limit(obj, darea->points))
					{
						memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
						darea->step = step;

						return FALSE;
					}

#if 0
					ppoint = (GdkPoint*)imalloc(sizeof(GdkPoint) * MAX_POINTS);
					sort_points(ppoint, darea->points);

					darea->list = g_slist_append(darea->list, ppoint);
					darea->list_length++;

					memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
#endif
					if(darea->list_length < MAX_AREA_COUNT) {
						ppoint = (GdkPoint*)imalloc(sizeof(GdkPoint) * MAX_POINTS);
						sort_points(ppoint, darea->points);

						darea->list = g_slist_append(darea->list, ppoint);
						darea->list_length++;
					}else {
						ppoint = (GdkPoint*)g_slist_nth_data(darea->list, 0);
						sort_points(ppoint, darea->points);
					}

					memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
				}
			}
			break;

		default:
			memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
			darea->step = DRAW_NONE_STEP;
			darea->area_state = NORMAL_STATE;
			darea->focus_area = -1;
			darea->resize_side = NONE_SIDE;
			return FALSE;
	}

	darea->step = step;

	return TRUE;
}

static gboolean modify_points(NFOBJECT *obj, gint x, gint y)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	gint x_gap, y_gap;
	gint off_x, off_y;
	gint over_x, over_y;
	GdkRectangle rect;

	x_gap = y_gap = 0;

	rect.x = darea->points[0].x + (LINE_WIDTH / 2);
	rect.y = darea->points[0].y + (LINE_WIDTH / 2);
	rect.width = darea->points[2].x - darea->points[0].x - (LINE_WIDTH / 2 * 2);
	rect.height = darea->points[2].y - darea->points[0].y - (LINE_WIDTH / 2 * 2);

#if 0
	rect.x = darea->points[0].x;
	rect.y = darea->points[0].y;
	rect.width = darea->points[2].x - darea->points[0].x;
	rect.height = darea->points[2].y - darea->points[0].y;
#endif

	// move
	if(darea->resize_side == NONE_SIDE && check_coord_in_rect(rect, x, y)) 
	{
		x_gap = ((darea->points[2].x - darea->points[0].x)/2);
		y_gap = ((darea->points[2].y - darea->points[0].y)/2);

		darea->points[0].x = x - x_gap; 
		darea->points[0].y = y - y_gap;
		darea->points[1].x = x + x_gap; 
		darea->points[1].y = y - y_gap;
		darea->points[2].x = x + x_gap; 
		darea->points[2].y = y + y_gap;
		darea->points[3].x = x - x_gap;
		darea->points[3].y = y + y_gap;
		darea->points[4].x = x - x_gap;
		darea->points[4].y = y - y_gap;

		// check x, y coord limit.
		over_x = over_y = 0;
		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		if(off_x > darea->points[0].x) 
		{
			over_x = off_x - darea->points[0].x;
		}
		else if(off_x + obj->width < darea->points[1].x)
		{
			over_x = darea->points[1].x - (off_x + obj->width);
			over_x *= -1;
		}

		darea->points[0].x += over_x; 
		darea->points[1].x += over_x; 
		darea->points[2].x += over_x; 
		darea->points[3].x += over_x;
		darea->points[4].x += over_x;

		if(off_y > darea->points[0].y) 
		{
			over_y = off_y - darea->points[0].y;
		}
		else if(off_y + obj->height < darea->points[3].y)
		{
			over_y = darea->points[3].y - (off_y + obj->height);
			over_y *= -1;
		}

		darea->points[0].y += over_y;
		darea->points[1].y += over_y;
		darea->points[2].y += over_y;
		darea->points[3].y += over_y;
		darea->points[4].y += over_y;

		return TRUE;
	}
	// resize
	else
	{
		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		if(x <= off_x || x >= (off_x + obj->width))
			return FALSE;

		if(y <= off_y || y >= (off_y + obj->height))
			return FALSE;

		// top left
		rect.x = darea->points[0].x - OUTRECT_GAP;
		rect.y = darea->points[0].y - OUTRECT_GAP;
		rect.width = rect.height = (LINE_WIDTH+OUTRECT_SIZE);
		if(darea->resize_side == TOP_LEFT || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[0].x = x; 
			darea->points[0].y = y;
			darea->points[1].y = y;
			darea->points[3].x = x;
			darea->points[4].x = x;
			darea->points[4].y = y;

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = TOP_LEFT;

			return TRUE;
		}

		// top middle 
		rect.x = darea->points[0].x + (darea->points[1].x - darea->points[0].x)/2 - OUTRECT_SIZE/2;
		rect.y = darea->points[0].y - OUTRECT_GAP;
		rect.width = rect.height = OUTRECT_SIZE;
		if(darea->resize_side == TOP_MIDDLE || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[0].y = y;
			darea->points[1].y = y;
			darea->points[4].y = y;

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = TOP_MIDDLE;

			return TRUE;
		}

		// top right
		//rect.x = darea->points[1].x + 2;
		rect.x = darea->points[1].x - (LINE_WIDTH/2);
		rect.y = darea->points[1].y - OUTRECT_GAP;
		rect.width = rect.height = (LINE_WIDTH+OUTRECT_SIZE);
		if(darea->resize_side == TOP_RIGHT || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[0].y = y;
			darea->points[1].x = x; 
			darea->points[1].y = y;
			darea->points[2].x = x; 
			darea->points[4].y = y;

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = TOP_RIGHT;

			return TRUE;
		}

		// middle right
		//rect.x = darea->points[1].x + 2;
		rect.x = darea->points[1].x + (LINE_WIDTH/2);
		rect.y = darea->points[1].y + (darea->points[2].y - darea->points[1].y)/2 - OUTRECT_SIZE/2;
		rect.width = rect.height = OUTRECT_SIZE;
		if(darea->resize_side == MIDDLE_RIGHT || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[1].x = x; 
			darea->points[2].x = x; 

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = MIDDLE_RIGHT;

			return TRUE;
		}

		// bottom right
		//rect.x = darea->points[2].x + 2;
		//rect.y = darea->points[2].y + 2;
		rect.x = darea->points[2].x - (LINE_WIDTH/2);
		rect.y = darea->points[2].y - (LINE_WIDTH/2);
		rect.width = rect.height = (LINE_WIDTH+OUTRECT_SIZE);
		if(darea->resize_side == BOTTOM_RIGHT || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[1].x = x; 
			darea->points[2].x = x; 
			darea->points[2].y = y;
			darea->points[3].y = y;

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = BOTTOM_RIGHT;

			return TRUE;
		}

		// bottom middle
		rect.x = darea->points[3].x + (darea->points[2].x - darea->points[3].x)/2 - OUTRECT_SIZE/2;
		//rect.y = darea->points[3].y + 2;
		rect.y = darea->points[3].y + (LINE_WIDTH/2);
		rect.width = rect.height = OUTRECT_SIZE;
		if(darea->resize_side == BOTTOM_MIDDLE || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[2].y = y;
			darea->points[3].y = y;

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = BOTTOM_MIDDLE;

			return TRUE;
		}

		// bottom left
		rect.x = darea->points[3].x - OUTRECT_GAP;
		//rect.y = darea->points[3].y + 2;
		rect.y = darea->points[3].y - (LINE_WIDTH/2);
		rect.width = rect.height = (LINE_WIDTH+OUTRECT_SIZE);
		if(darea->resize_side == BOTTOM_LEFT || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[0].x = x; 
			darea->points[2].y = y;
			darea->points[3].x = x; 
			darea->points[3].y = y;
			darea->points[4].x = x; 

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = BOTTOM_LEFT;

			return TRUE;
		}

		// middle left
		rect.x = darea->points[0].x - OUTRECT_GAP;
		rect.y = darea->points[0].y + (darea->points[3].y - darea->points[0].y)/2 - OUTRECT_SIZE/2;
		rect.width = rect.height = OUTRECT_SIZE;
		if(darea->resize_side == MIDDLE_LEFT || check_coord_in_rect(rect, x, y)) 
		{
			darea->points[0].x = x; 
			darea->points[3].x = x; 
			darea->points[4].x = x; 

			if(darea->resize_side == NONE_SIDE)
				darea->resize_side = MIDDLE_LEFT;

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean draw_area(NFOBJECT *obj, GdkWindow *wnd, GdkLineStyle line_style, gint state)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	GdkGC *gc;

	if(darea->step != DRAW_EDIT_STEP && darea->step != DRAW_MODIFY_STEP)
		return FALSE;

	gc = nfui_nfobject_get_gc(obj);

	if(state == NORMAL_STATE) 		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(732));
	else if(state == FOCUS_STATE) 	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(733));
	else 							gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(734));
	gdk_gc_set_line_attributes(gc, LINE_WIDTH, line_style, GDK_CAP_NOT_LAST, GDK_JOIN_MITER); 

	gdk_draw_lines(wnd, gc, darea->points, MAX_POINTS);

	nfui_nfobject_gc_unref(gc);

	return TRUE;
}

static gboolean redraw_all_area(NFOBJECT *obj, GdkWindow *wnd)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	GdkDrawable *drawable = NULL;
	GdkPoint *ppoint;
	GdkGC *gc;
	guint i;

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(732));
	gdk_gc_set_line_attributes(gc, LINE_WIDTH, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER); 

	for(i=0; i<darea->list_length; i++) 
	{
		if(darea->step == DRAW_MODIFY_STEP && darea->focus_area == (gint)i) 
			continue;

		ppoint = (GdkPoint*)g_slist_nth_data(darea->list, i);
		gdk_draw_lines(drawable, gc, ppoint, MAX_POINTS);
	}

	nfui_nfobject_gc_unref(gc);

	return TRUE;
}

static gboolean draw_resizing_area(NFOBJECT *obj, GdkWindow *wnd)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	gint off_x, off_y, size_w, size_h;
	GdkGC *gc;

	if(darea->step != DRAW_EDIT_STEP && darea->step != DRAW_MODIFY_STEP)
		return FALSE;

	gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(732));

	nfui_nfobject_get_offset(obj, &off_x, &off_y);
	size_w = obj->width;
	size_h = obj->height;

	if(darea->points[0].x - OUTRECT_GAP > off_x && darea->points[0].y - OUTRECT_GAP > off_y)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[0].x - OUTRECT_GAP, darea->points[0].y - OUTRECT_GAP, OUTRECT_SIZE, OUTRECT_SIZE); 														// top left

	if(darea->points[0].y - OUTRECT_GAP > off_y)
		gdk_draw_rectangle(wnd, gc, TRUE, (darea->points[0].x + (darea->points[1].x - darea->points[0].x)/2) - OUTRECT_SIZE/2, darea->points[0].y - OUTRECT_GAP, OUTRECT_SIZE, OUTRECT_SIZE); 	// top middle 

	if(darea->points[1].x + 2 + OUTRECT_SIZE < off_x + size_w && darea->points[1].y - OUTRECT_GAP > off_y)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[1].x + 2, darea->points[1].y - OUTRECT_GAP, OUTRECT_SIZE, OUTRECT_SIZE); 																// top right

	if(darea->points[1].x + 2 + OUTRECT_SIZE < off_x + size_w)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[1].x + 2, (darea->points[1].y + (darea->points[2].y - darea->points[1].y)/2) - OUTRECT_SIZE/2, OUTRECT_SIZE, OUTRECT_SIZE); 			// middle right

	if(darea->points[2].x + 2 + OUTRECT_SIZE < off_x + size_w && darea->points[2].y + 2 + OUTRECT_SIZE < off_y + size_h)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[2].x + 2, darea->points[2].y + 2, OUTRECT_SIZE, OUTRECT_SIZE); 																			// bottom right

	if(darea->points[3].y + 2 + OUTRECT_SIZE < off_y + size_h)
		gdk_draw_rectangle(wnd, gc, TRUE, (darea->points[3].x + (darea->points[2].x - darea->points[3].x)/2) - OUTRECT_SIZE/2, darea->points[3].y + 2, OUTRECT_SIZE, OUTRECT_SIZE); 			// bottom middle

	if(darea->points[3].x - OUTRECT_GAP > off_x && darea->points[3].y + 2 + OUTRECT_SIZE < off_y + size_h)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[3].x - OUTRECT_GAP, darea->points[3].y + 2, OUTRECT_SIZE, OUTRECT_SIZE); 																// bottom left

	if(darea->points[0].x - OUTRECT_GAP > off_x)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[0].x - OUTRECT_GAP, (darea->points[0].y + (darea->points[3].y - darea->points[0].y)/2) - OUTRECT_SIZE/2, OUTRECT_SIZE, OUTRECT_SIZE); 	// middle left

	nfui_nfobject_gc_unref(gc);

	return TRUE;
}

static gboolean erase_cur_area(NFOBJECT *obj, GdkWindow *wnd, GdkLineStyle line_style)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	GdkGC *gc;
	gint cidx;

	if(darea->step != DRAW_EDIT_STEP && darea->step != DRAW_MODIFY_STEP)
		return FALSE;

	gc = nfui_nfobject_get_gc(obj);

	cidx = nfui_nfobject_get_bg_color(obj, NFOBJECT_STATE_NORMAL);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(cidx));

	gdk_draw_lines(wnd, gc, darea->points, MAX_POINTS);

	nfui_nfobject_gc_unref(gc);

	return TRUE;
}

static gboolean erase_pre_area(NFOBJECT *obj, guint pre_num, GdkWindow *wnd, GdkLineStyle line_style)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	GdkPoint *ppoint;
	GdkGC *gc;
	gint cidx;

	if(darea->step != DRAW_EDIT_STEP && darea->step != DRAW_MODIFY_STEP)
		return FALSE;

	if(darea->list_length < pre_num)
		return FALSE;

	gc = nfui_nfobject_get_gc(obj);

	cidx = nfui_nfobject_get_bg_color(obj, NFOBJECT_STATE_NORMAL);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(cidx));

	ppoint = (GdkPoint*)g_slist_nth_data(darea->list, pre_num);
	gdk_draw_lines(wnd, gc, ppoint, MAX_POINTS);

	nfui_nfobject_gc_unref(gc);

	return TRUE;
}

static gboolean erase_resizing_area(NFOBJECT *obj, GdkWindow *wnd)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	GdkGC *gc;
	gint off_x, off_y, size_w, size_h;
	gint cidx;

	if(darea->step != DRAW_EDIT_STEP && darea->step != DRAW_MODIFY_STEP)
		return FALSE;

	gc = nfui_nfobject_get_gc(obj);

	nfui_nfobject_get_offset(obj, &off_x, &off_y);
	size_w = obj->width;
	size_h = obj->height;

	cidx = nfui_nfobject_get_bg_color(obj, NFOBJECT_STATE_NORMAL);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(cidx));

	if(darea->points[0].x - OUTRECT_GAP > off_x && darea->points[0].y - OUTRECT_GAP > off_y)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[0].x - OUTRECT_GAP, darea->points[0].y - OUTRECT_GAP, OUTRECT_SIZE, OUTRECT_SIZE); 														// top left

	if(darea->points[0].y - OUTRECT_GAP > off_y)
		gdk_draw_rectangle(wnd, gc, TRUE, (darea->points[0].x + (darea->points[1].x - darea->points[0].x)/2) - OUTRECT_SIZE/2, darea->points[0].y - OUTRECT_GAP, OUTRECT_SIZE, OUTRECT_SIZE); 	// top middle 

	if(darea->points[1].x + 2 + OUTRECT_SIZE < off_x + size_w && darea->points[1].y - OUTRECT_GAP > off_y)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[1].x + 2, darea->points[1].y - OUTRECT_GAP, OUTRECT_SIZE, OUTRECT_SIZE); 																// top right

	if(darea->points[1].x + 2 + OUTRECT_SIZE < off_x + size_w)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[1].x + 2, (darea->points[1].y + (darea->points[2].y - darea->points[1].y)/2) - OUTRECT_SIZE/2, OUTRECT_SIZE, OUTRECT_SIZE); 			// middle right

	if(darea->points[2].x + 2 + OUTRECT_SIZE < off_x + size_w && darea->points[2].y + 2 + OUTRECT_SIZE < off_y + size_h)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[2].x + 2, darea->points[2].y + 2, OUTRECT_SIZE, OUTRECT_SIZE); 																			// bottom right

	if(darea->points[3].y + 2 + OUTRECT_SIZE < off_y + size_h)
		gdk_draw_rectangle(wnd, gc, TRUE, (darea->points[3].x + (darea->points[2].x - darea->points[3].x)/2) - OUTRECT_SIZE/2, darea->points[3].y + 2, OUTRECT_SIZE, OUTRECT_SIZE); 			// bottom middle

	if(darea->points[3].x - OUTRECT_GAP > off_x && darea->points[3].y + 2 + OUTRECT_SIZE < off_y + size_h)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[3].x - OUTRECT_GAP, darea->points[3].y + 2, OUTRECT_SIZE, OUTRECT_SIZE); 																// bottom left

	if(darea->points[0].x - OUTRECT_GAP > off_x)
		gdk_draw_rectangle(wnd, gc, TRUE, darea->points[0].x - OUTRECT_GAP, (darea->points[0].y + (darea->points[3].y - darea->points[0].y)/2) - OUTRECT_SIZE/2, OUTRECT_SIZE, OUTRECT_SIZE); 	// middle left

	nfui_nfobject_gc_unref(gc);

	return TRUE;
}

static gboolean nfdrawarea_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFDRAWAREA *darea = (NFDRAWAREA*)obj;
	GdkModifierType state;
	gint x, y;

	switch(event->type) 
	{
		case GDK_EXPOSE:
			{
				GdkEventExpose *eevt = (GdkEventExpose*)event;

				redraw_all_area(obj, eevt->window);
			}
			break;

		case GDK_BUTTON_PRESS:
			{
				GdkEventButton *bevt;

				bevt = (GdkEventButton*)event;
				gdk_window_get_pointer(bevt->window, &x, &y, &state);

				if(state & GDK_BUTTON1_MASK)
				{
					if(darea->step == DRAW_MODIFY_STEP) 
					{
						nftool_change_custom_cursor(bevt->window, NF_CURSOR_FLEUR);
						break;
					}
					set_points(obj, DRAW_START_STEP, x, y);
				}
			}
			break;

		case GDK_2BUTTON_PRESS:
			{
				GdkEventButton *bevt;

				bevt = (GdkEventButton*)event;
				gdk_window_get_pointer(bevt->window, &x, &y, &state);

				if(!(state & GDK_BUTTON1_MASK))
					break;
				
				if(darea->step == DRAW_MODIFY_STEP) 
				{
					draw_area(obj, bevt->window, GDK_LINE_SOLID, NORMAL_STATE);
					erase_resizing_area(obj, bevt->window);
					set_points(obj, DRAW_END_STEP, -1, -1);

					redraw_all_area(obj, bevt->window);
					nftool_change_custom_cursor(bevt->window, NF_CURSOR_ARROW);
				}
			}
			break;

		case GDK_BUTTON_RELEASE:
			{
				GdkEventButton *bevt;

				bevt = (GdkEventButton*)event;
				gdk_window_get_pointer(bevt->window, &x, &y, &state);

				if(darea->step == DRAW_START_STEP) 
				{
					set_points(obj, DRAW_MODIFY_STEP, x, y);
					draw_area(obj, bevt->window, GDK_LINE_SOLID, FOCUS_STATE);
					draw_resizing_area(obj, bevt->window);
				}
				else if(darea->step == DRAW_MODIFY_STEP) 
				{
					draw_area(obj, bevt->window, GDK_LINE_SOLID, NORMAL_STATE);
					erase_resizing_area(obj, bevt->window);
					set_points(obj, DRAW_END_STEP, -1, -1);

					redraw_all_area(obj, bevt->window);
					nftool_change_custom_cursor(bevt->window, NF_CURSOR_ARROW);

					if(set_points(obj, DRAW_MODIFY_STEP, x, y))
					{
						draw_area(obj, bevt->window, GDK_LINE_SOLID, FOCUS_STATE);
						draw_resizing_area(obj, bevt->window);
					}
				}
				else 
				{
#if 0
					if(darea->list_length == MAX_AREA_COUNT)
					{
						erase_cur_area(obj, bevt->window, GDK_LINE_ON_OFF_DASH);
						set_points(obj, DRAW_NONE_STEP, -1, -1);
						redraw_all_area(obj, bevt->window);
						break;
					}
#endif
					if(darea->list_length == MAX_AREA_COUNT)
						erase_pre_area(obj, 0, bevt->window, GDK_LINE_SOLID);

					draw_area(obj, bevt->window, GDK_LINE_SOLID, NORMAL_STATE);

					if(darea->area_state == FOCUS_STATE)
						erase_resizing_area(obj, bevt->window);

					if(!check_area_size_limit(obj, darea->points)) {
						erase_cur_area(obj, bevt->window, GDK_LINE_SOLID);
						redraw_all_area(obj, bevt->window);
					}

					set_points(obj, DRAW_END_STEP, -1, -1);
				}
			}
			break;

		case GDK_MOTION_NOTIFY:
			{
				GdkEventMotion *mevt;
				
				mevt = (GdkEventMotion*)event;
				gdk_window_get_pointer(mevt->window, &x, &y, &state);

				if(state & GDK_BUTTON1_MASK) {
					erase_cur_area(obj, mevt->window, GDK_LINE_ON_OFF_DASH);

					if(darea->step == DRAW_MODIFY_STEP) 
					{
						erase_resizing_area(obj, mevt->window);

						if(modify_points(obj, x, y)) 
						{
							draw_area(obj, mevt->window, GDK_LINE_ON_OFF_DASH, darea->area_state);
						}
						else 
						{
							draw_area(obj, mevt->window, GDK_LINE_SOLID, NORMAL_STATE);
							set_points(obj, DRAW_END_STEP, -1, -1);
							nftool_change_custom_cursor(mevt->window, NF_CURSOR_ARROW);

							set_points(obj, DRAW_START_STEP, x, y);
							set_points(obj, DRAW_EDIT_STEP, x, y);
							draw_area(obj, mevt->window, GDK_LINE_ON_OFF_DASH, darea->area_state);
						}
					}
					else 
					{
						set_points(obj, DRAW_EDIT_STEP, x, y);
						draw_area(obj, mevt->window, GDK_LINE_ON_OFF_DASH, darea->area_state);
					}

					redraw_all_area(obj, mevt->window);
				}
			}
			break;

		case GDK_ENTER_NOTIFY:
			break;

		case GDK_LEAVE_NOTIFY:
			{
				GdkDrawable *drawable = NULL;

				drawable = nfui_nfobject_get_window(obj);

				if(darea->step == DRAW_MODIFY_STEP) 
				{
					draw_area(obj, drawable, GDK_LINE_SOLID, NORMAL_STATE);
					erase_resizing_area(obj, drawable);
					set_points(obj, DRAW_END_STEP, -1, -1);

					nftool_change_custom_cursor(drawable, NF_CURSOR_ARROW);
				}
				else
				{
					erase_cur_area(obj, drawable, GDK_LINE_SOLID);
					set_points(obj, DRAW_NONE_STEP, -1, -1);
					redraw_all_area(obj, drawable);
				}
			}
			break;

		case GDK_DELETE:
			{
				GdkPoint *p;
				guint i;

				for(i=0; i<darea->list_length; i++) 
				{
					p = (GdkPoint*)g_slist_nth_data(darea->list, i);
					g_free(p);
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}




NFOBJECT* nfui_drawrect_area_new()
{
	NFDRAWAREA	*darea;

	darea = (NFDRAWAREA*)imalloc(sizeof(NFDRAWAREA));
	if(!darea) 
	{	
		g_error("NFDRAWAREA alloc error...");
		return NULL;
	}

	nfui_nfobject_init((NFOBJECT*)darea);

	darea->object.width = 0;
	darea->object.height = 0;
	darea->object.type = NFOBJECT_TYPE_DRAWAREA;
	darea->object.use_focus = NFOBJECT_FOCUS_ON;
	darea->object.default_event_handler = nfdrawarea_event_handler;

	memset(darea->points, -1, sizeof(GdkPoint) * MAX_POINTS);
	darea->list_length = 0;

	darea->step = DRAW_NONE_STEP;
	darea->area_state = NORMAL_STATE;
	darea->focus_area = -1;
	darea->resize_side = NONE_SIDE;

	return (NFOBJECT*)darea;
}

gboolean nfui_drawrect_set_rect(NFDRAWAREA* darea, GdkRectangle rect)
{
	GdkPoint *ppoint;
	gint x, y, w, h;

	g_return_val_if_fail(darea->object.type == NFOBJECT_TYPE_DRAWAREA, FALSE);
	
	x = rect.x;
	y = rect.y;
	w = rect.width;
	h = rect.height;

	g_return_val_if_fail(w >= MIN_AREA_SIZE_W, FALSE);
	g_return_val_if_fail(h >= MIN_AREA_SIZE_H, FALSE);

	if(darea->list_length < MAX_AREA_COUNT) {
		ppoint = (GdkPoint*)imalloc(sizeof(GdkPoint) * MAX_POINTS);

		darea->list = g_slist_append(darea->list, ppoint);
		darea->list_length++;
	}else {
		ppoint = (GdkPoint*)g_slist_nth_data(darea->list, 0);
	}

	ppoint[0].x = x; 	
	ppoint[0].y = y;
	ppoint[1].x = x + w; 
	ppoint[1].y = y;
	ppoint[2].x = x + w; 
	ppoint[2].y = y + h;
	ppoint[3].x = x; 	
	ppoint[3].y = y + h;
	ppoint[4].x = x; 	
	ppoint[4].y = y;

	return TRUE;
}

gboolean nfui_drawrect_get_rect(NFDRAWAREA* darea, GdkRectangle *rect)
{
	GdkPoint *ppoint;

	g_return_val_if_fail(darea->object.type == NFOBJECT_TYPE_DRAWAREA, FALSE);
	
	ppoint = (GdkPoint*)g_slist_nth_data(darea->list, 0);

	rect->x = ppoint[0].x;
	rect->y = ppoint[0].y;
	rect->width = ppoint[2].x - ppoint[0].x; 
	rect->height = ppoint[2].y - ppoint[0].y; 

	return TRUE;
}
