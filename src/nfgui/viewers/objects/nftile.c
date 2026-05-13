#include <string.h>

#include "../../support/util.h"
#include "../../support/event_loop.h"
#include "../../support/color.h"


#include "nftile.h"
#include "nfscrolledfixed.h"
#include "ix_mem.h"
#include "iux_afx.h"


#define DBG_LEVEL		0
#define DBG_MODULE		"NFTILE"


#define START_ROW(r1, r2)					(r1 > r2 ? r2 : r1)
#define END_ROW(r1, r2)						(r1 > r2 ? r1 : r2)
#define START_COL(c1, c2)					(c1 > c2 ? c2 : c1)
#define END_COL(c1, c2)						(c1 > c2 ? c1 : c2)

#define TILE_LINE_FILLED_W					1							// Filled tile
#define TILE_LINE_NO_FILLED_W				tile->line_border			// No filled tile
#define TILE_OUTLINE_W						2							// Tile outline

#define SELECT_COLOR_START_INDEX			2
#define SELECT_EX_COLOR_START_INDEX			6



static gboolean nftile_event_handler(NFTILE *tile, GdkEvent *event, gpointer data);

static void nftile_init(NFTILE *tile);
static void nftile_draw_lines(NFTILE *tile);
static void nftile_draw_color(NFTILE *tile, NFTileArea *area, GdkDrawable *drawable, NFTileState state, gboolean force);
static NFTileArea* nftile_get_area(NFTILE *tile, guint x, guint y);
static void nftile_draw_outlines(NFOBJECT *obj);
static void nftile_key_move(NFTILE *tile, GdkDrawable *drawable, KEYPAD_KID dir);
static void nftile_end_selecting(NFTILE *tile);
static void nftile_change_current_state(NFTILE *tile, NFTileState old, NFTileState new, gboolean redraw);


static gboolean nftile_event_handler(NFTILE *tile, GdkEvent *event, gpointer data)
{
	NFTileArea *area = NULL;
	GdkDrawable *drawable;
	guint sr, er, sc, ec;
	guint x, y;
	guint i, j;

	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

	switch(event->type) {
		case GDK_EXPOSE:
			{
				if(tile->object.kfocus != NFOBJECT_FOCUS)
					tile->object.grab_kfocus = FALSE;

				if(!tile->tarea) {
					nftile_init(tile);
					nfui_user_signal_emit((NFOBJECT*)tile, NFEVENT_TILE_INIT, FALSE);
				}

				//nftile_draw_lines(tile);

				for(i=0; i<tile->row_size; i++)
					for(j=0; j<tile->col_size; j++)
						nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, TRUE);

				if(tile->draw_outline && tile->object.kfocus == NFOBJECT_FOCUS)
					nftile_draw_outlines((NFOBJECT*)tile);
			}
			break;
		case GDK_MOTION_NOTIFY:
			{
				GdkEventMotion *mevent;
				NFTileArea *start = NULL, *end = NULL;
				NFTileState area_calc[60][60] = {NFTILE_STATE_NONE, };


				if(!tile->sensitive)
					break;

				if(!tile->tarea)
					break;

				if(tile->pushed_enter) {
					nftile_end_selecting(tile);

					tile->pushed_enter = FALSE;

					tile->start_area = tile->end_area;
				}

				mevent = (GdkEventMotion*)event;
				if(mevent->state & GDK_BUTTON3_MASK)
					break;

				x = (guint)mevent->x;
				y = (guint)mevent->y;

				start = tile->start_area;
				end = tile->end_area;
				if(x && y)
					area = nftile_get_area(tile, x, y);
				else
					break;

				if(tile->innout == POINTER_INNOUT) {
					if(!(mevent->state & (1 << 8)))
						tile->pressed = FALSE;
				}

				if(area) {
					if(tile->pressed) {
						if(end != area) {
							sr = START_ROW(start->row, end->row);
							er = END_ROW(start->row, end->row);
							sc = START_COL(start->col, end->col);
							ec = END_COL(start->col, end->col);

							for(i = sr; i <= er; i++) {
								for(j = sc; j <= ec; j++) {
									if(tile->tarea[i][j].state & NFTILE_STATE_FOCUS) {
										tile->tarea[i][j].state &= ~NFTILE_STATE_FOCUS;
										tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;
									}

									area_calc[i][j] = tile->tarea[i][j].state;
								}
							}
						}

						sr = START_ROW(start->row, area->row);
						er = END_ROW(start->row, area->row);
						sc = START_COL(start->col, area->col);
						ec = END_COL(start->col, area->col);

						for(i = sr; i <= er; i++) {
							for(j = sc; j <= ec; j++) {
								if(tile->tarea[i][j].state & NFTILE_STATE_NORMAL) {
									tile->tarea[i][j].state &= ~NFTILE_STATE_NORMAL;
									tile->tarea[i][j].state |= NFTILE_STATE_FOCUS;

									area_calc[i][j] = tile->tarea[i][j].state;

								}else if(tile->tarea[i][j].state & NFTILE_STATE_SELECT
											|| tile->tarea[i][j].state & NFTILE_STATE_RESELECT) {
									area_calc[i][j] = NFTILE_STATE_FOCUS;
								}
							}
						}

						for(i=0; i<tile->row_size; i++)
							for(j=0; j<tile->col_size; j++)
							{
								if (area_calc[i][j] != NFTILE_STATE_NONE)
									nftile_draw_color(tile, &tile->tarea[i][j], drawable, area_calc[i][j], FALSE);
							}

						tile->end_area = area;

					}else {
						if(tile->innout == POINTER_INNOUT) {
							sr = START_ROW(start->row, end->row);
							er = END_ROW(start->row, end->row);
							sc = START_COL(start->col, end->col);
							ec = END_COL(start->col, end->col);

							for(i = sr; i <= er; i++) {
								for(j = sc; j <= ec; j++) {
									if(tile->tarea[i][j].state & NFTILE_STATE_FOCUS) {
										tile->tarea[i][j].state &= ~NFTILE_STATE_FOCUS;
										tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;
									}

									nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);

									tile->innout = 0;
								}
							}
						}

						if(area->state & NFTILE_STATE_FOCUS)
							break;

						if(start != area) {
							if(end) {
								if(end->state & NFTILE_STATE_FOCUS) {
									end->state &= ~NFTILE_STATE_FOCUS;
									end->state |= NFTILE_STATE_NORMAL;

									nftile_draw_color(tile, end, drawable, end->state, FALSE);
								}else if(end->state & NFTILE_STATE_SELECT
										|| end->state & NFTILE_STATE_RESELECT) {

									nftile_draw_color(tile, end, drawable, end->state, FALSE);
								}
							}

							if(area->state & NFTILE_STATE_NORMAL) {
								area->state &= ~NFTILE_STATE_NORMAL;
								area->state |= NFTILE_STATE_FOCUS;

								tile->start_area = tile->end_area = area;

								nftile_draw_color(tile, tile->start_area, drawable, tile->start_area->state, FALSE);
							}else if(area->state & NFTILE_STATE_SELECT
										|| area->state & NFTILE_STATE_RESELECT) {

								tile->start_area = tile->end_area = area;
								nftile_draw_color(tile, tile->start_area, drawable, NFTILE_STATE_FOCUS, FALSE);
							}
						}
						else {
							if(area->state & NFTILE_STATE_NORMAL) {
								area->state &= ~NFTILE_STATE_NORMAL;
								area->state |= NFTILE_STATE_FOCUS;

								nftile_draw_color(tile, tile->start_area, drawable, tile->start_area->state, FALSE);
							}else if(area->state & NFTILE_STATE_SELECT
										|| area->state & NFTILE_STATE_RESELECT) {
								nftile_draw_color(tile, area, drawable, NFTILE_STATE_FOCUS, FALSE);
							}
						}
					}
					nfui_user_signal_emit((NFOBJECT*)tile, NFEVENT_TILE_MOVE_SELECT, FALSE);

					if(tile->draw_outline && tile->object.kfocus == NFOBJECT_FOCUS)
						nftile_draw_outlines((NFOBJECT*)tile);
				}
			}
			break;
		case GDK_BUTTON_PRESS:
			{
				GdkEventButton *bevent;
				gint pre_sr, pre_sc;

				if(!tile->sensitive)
					break;

				if(!tile->tarea)
					break;

				bevent = (GdkEventButton*)event;
				if(bevent->button != 1) { 		// left mouse button
					break;
				}

				if(!tile->pressed)
					tile->pressed = TRUE;

				x = (guint)bevent->x;
				y = (guint)bevent->y;

				area = nftile_get_area(tile, x, y);
				if(area) {
					pre_sr = tile->start_area->row;
					pre_sc = tile->start_area->col;

					// draw pre area state
					if((pre_sr != area->row) || (pre_sc != area->col)) {
						tile->tarea[pre_sr][pre_sc].state &= ~NFTILE_STATE_FOCUS;

						if(tile->tarea[pre_sr][pre_sc].state == NFTILE_STATE_NONE)
							tile->tarea[pre_sr][pre_sc].state |= NFTILE_STATE_NORMAL;

						nftile_draw_color(tile, &tile->tarea[pre_sr][pre_sc], drawable, tile->tarea[pre_sr][pre_sc].state, FALSE);
					}

					tile->start_area = tile->end_area = area;

					nfui_user_signal_emit((NFOBJECT*)tile, NFEVENT_TILE_START_SELECT, FALSE);
				}
			}
			break;
		case GDK_BUTTON_RELEASE:
			{
				GdkEventButton *bevent;

				if(!tile->sensitive)
					break;

				if(!tile->tarea)
					break;

				bevent = (GdkEventButton*)event;
				if(bevent->button != 1) {			// left mouse button
					if(!tile->pressed)
						break;
				}

				if(tile->pressed)
					tile->pressed = FALSE;

				x = (guint)bevent->x;
				y = (guint)bevent->y;

				area = nftile_get_area(tile, x, y);
				if(area) {
					switch(area->state) {
						case NFTILE_STATE_NONE:
							break;
						case NFTILE_STATE_NORMAL:
						case NFTILE_STATE_FOCUS:
						case NFTILE_STATE_SELECT:
						case NFTILE_STATE_RESELECT:
							{
								nftile_end_selecting(tile);
							}
							break;
					}
				}
			}
			break;
		case GDK_ENTER_NOTIFY:
			{
				if(tile->pressed) {
					if(!(tile->innout & POINTER_IN_TILE))
						tile->innout |= POINTER_IN_TILE;
				}

				if(tile->draw_outline && nfui_nfobject_is_shown((NFOBJECT*)tile))
					nfui_signal_emit((NFOBJECT*)tile, GDK_EXPOSE, FALSE);
    			break;

			}
			break;
		case GDK_LEAVE_NOTIFY:
			{
				if(!tile->sensitive)
					break;

				if(tile->pressed) {
					if(!(tile->innout & POINTER_OUT_TILE))
						tile->innout |= POINTER_OUT_TILE;
				}

				if(tile->draw_outline && nfui_nfobject_is_shown((NFOBJECT*)tile))
					nfui_signal_emit((NFOBJECT*)tile, GDK_EXPOSE, FALSE);
			}
			break;
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;
				NFTileState state = NFTILE_STATE_NONE;
				gint row, col;

				kevt = (GdkEventKey*)event;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(!tile->sensitive)
					break;

				if(!tile->tarea)
					break;

				switch(kpid) {
					case KEYPAD_ENTER:
						{
							if(!tile->object.grab_kfocus)
							{
								tile->object.grab_kfocus = TRUE;

								if(tile->start_area != tile->end_area){
									sr = START_ROW(tile->start_area->row, tile->end_area->row);
									er = END_ROW(tile->start_area->row, tile->end_area->row);
									sc = START_COL(tile->start_area->col, tile->end_area->col);
									ec = END_COL(tile->start_area->col, tile->end_area->col);

									for(i = sr; i <= er; i++) {
										for(j = sc; j <= ec; j++) {
											if(tile->tarea[i][j].state & NFTILE_STATE_FOCUS) {
												tile->tarea[i][j].state &= ~NFTILE_STATE_FOCUS;
												tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;
											}
											nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);
										}
									}
								tile->start_area = tile->end_area;
								}

								row = tile->start_area->row;
								col = tile->start_area->col;

								if(tile->tarea[row][col].state & NFTILE_STATE_NORMAL){
									tile->tarea[row][col].state &= ~NFTILE_STATE_NORMAL;
									tile->tarea[row][col].state |= NFTILE_STATE_FOCUS;

									nftile_draw_color(tile, &tile->tarea[row][col], drawable, tile->tarea[row][col].state, FALSE);

									nfui_signal_emit((NFOBJECT*)tile, GDK_EXPOSE, TRUE);
								}else  {
									nfui_signal_emit((NFOBJECT*)tile, GDK_EXPOSE, TRUE);

									nftile_draw_color(tile, &tile->tarea[row][col], drawable, NFTILE_STATE_FOCUS, FALSE);
								}

								if(tile->pressed == TRUE)
									tile->pressed = FALSE;

							}
							else
							{
								if(tile->pushed_enter)
								{
									nftile_end_selecting(tile);

									tile->pushed_enter = FALSE;

									tile->object.grab_kfocus = FALSE;

									tile->start_area = tile->end_area;
								}
								else
								{
									row = tile->end_area->row;
									col = tile->end_area->col;

									nftile_draw_color(tile, &tile->tarea[row][col], drawable, NFTILE_STATE_FOCUS, FALSE);

									tile->pushed_enter = TRUE;

									if(tile->tarea[row][col].state & NFTILE_STATE_FOCUS)
										nfui_user_signal_emit((NFOBJECT*)tile, NFEVENT_TILE_START_SELECT, FALSE);
								}

								return TRUE;
							}
						}
						break;
					case KEYPAD_EXIT:
						{
							if(tile->object.grab_kfocus)
							{
								if(tile->pushed_enter)
								{
									tile->pushed_enter = FALSE;
									tile->innout = POINTER_INNOUT;

									nftile_change_current_state(tile, NFTILE_STATE_FOCUS, NFTILE_STATE_NORMAL, TRUE);
									tile->end_area = tile->start_area;
								}
								else
								{
									row = tile->start_area->row;
									col = tile->start_area->col;

									if(tile->tarea[row][col].state & NFTILE_STATE_FOCUS) {
										tile->tarea[row][col].state &= ~NFTILE_STATE_FOCUS;
										tile->tarea[row][col].state |= NFTILE_STATE_NORMAL;

										nftile_draw_color(tile, &tile->tarea[row][col], drawable, tile->tarea[row][col].state, FALSE);
									}

									tile->object.grab_kfocus = FALSE;

									nfui_signal_emit((NFOBJECT*)tile, GDK_EXPOSE, TRUE);
								}

								return TRUE;
							}
						}
						break;
					case KEYPAD_UP:
						{
							if(tile->object.grab_kfocus) {
								nftile_key_move(tile, drawable, KEYPAD_UP);

								return TRUE;
							}
						}
						break;
					case KEYPAD_DOWN:
						{
							if(tile->object.grab_kfocus) {
								nftile_key_move(tile, drawable, KEYPAD_DOWN);

								return TRUE;
							}
						}
						break;
					case KEYPAD_LEFT:
						{
							if(tile->object.grab_kfocus) {
								nftile_key_move(tile, drawable, KEYPAD_LEFT);

								return TRUE;
							}
						}
						break;

					case KEYPAD_RIGHT:
						{
							if(tile->object.grab_kfocus) {
								nftile_key_move(tile, drawable, KEYPAD_RIGHT);

								return TRUE;
							}
						}
						break;
					default:
						break;
				}
			}
			break;
		case GDK_DELETE:
			{
				int i;

				if(tile->tarea) {
					for(i = 0; i < tile->row_size; i++) {
						ifree(*(tile->tarea + i));
					}

					//				if(tile->tarea)
					ifree(tile->tarea);
					tile->tarea = NULL;
				}

				for(i=0; i<COLOR_COUNT; i++) {
					if(tile->filled) {
						if(tile->bg_gc[i]) {
							g_object_unref(tile->bg_gc[i]);
							tile->bg_gc[i] = NULL;
						}
					}else {
						if(tile->line_gc[i]) {
							g_object_unref(tile->line_gc[i]);
							tile->line_gc[i] = NULL;
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


static void nftile_end_selecting(NFTILE *tile)
{
	GdkDrawable *drawable;
	NFTileState state = NFTILE_STATE_NONE;
	guint sr, er, sc, ec;
	guint i, j;

	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

	sr = START_ROW(tile->start_area->row, tile->end_area->row);
	er = END_ROW(tile->start_area->row, tile->end_area->row);
	sc = START_COL(tile->start_area->col, tile->end_area->col);
	ec = END_COL(tile->start_area->col, tile->end_area->col);

	if(!tile->can_reselect)
	{
		if(tile->start_area->state & NFTILE_STATE_NORMAL || tile->start_area->state & NFTILE_STATE_FOCUS)
			state = NFTILE_STATE_SELECT;
		else if(tile->start_area->state & NFTILE_STATE_SELECT)
			state = NFTILE_STATE_NORMAL;
	}


	for(i = sr; i <= er; i++) {
		for(j = sc; j <= ec; j++) {
			if(tile->tarea[i][j].state & NFTILE_STATE_NORMAL)
			{
				tile->tarea[i][j].state &= ~NFTILE_STATE_NORMAL;
				tile->tarea[i][j].state |= state;
			}
			else if(tile->tarea[i][j].state & NFTILE_STATE_FOCUS)
			{
				tile->tarea[i][j].state &= ~NFTILE_STATE_FOCUS;

				if(tile->can_reselect)
				{
					tile->tarea[i][j].state |= NFTILE_STATE_SELECT;

					state |= NFTILE_STATE_SELECT;
				}
				else
					tile->tarea[i][j].state |= state;

			}
			else if(tile->tarea[i][j].state & NFTILE_STATE_SELECT)
			{
				if(tile->can_reselect)
				{
					tile->tarea[i][j].state &= ~NFTILE_STATE_SELECT;
					tile->tarea[i][j].state |= NFTILE_STATE_RESELECT;

					state |= NFTILE_STATE_RESELECT;
				}
				else
				{
					tile->tarea[i][j].state &= ~NFTILE_STATE_SELECT;
					tile->tarea[i][j].state |= state;
				}
			}
			else if(tile->tarea[i][j].state & NFTILE_STATE_RESELECT)
				state |= NFTILE_STATE_RESELECT;


			if(tile->tarea[i][j].state & NFTILE_STATE_SELECT || tile->tarea[i][j].state & NFTILE_STATE_RESELECT)
			{
				if(tile->can_reselect)
					tile->tarea[i][j].preSelect_color = tile->tarea[i][j].select_color;

				tile->tarea[i][j].select_color = tile->select_n;
			}

			nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);
		}
	}

	if(state & NFTILE_STATE_SELECT || state & NFTILE_STATE_RESELECT) {
		if(COLOR_COUNT <= ++tile->select_n) {
			tile->select_n = SELECT_COLOR_START_INDEX;
		}
	}

	nfui_user_signal_emit((NFOBJECT*)tile, NFEVENT_TILE_END_SELECT, FALSE);
}

static void nftile_change_current_state(NFTILE *tile, NFTileState old, NFTileState new, gboolean redraw)
{
	NFTileArea *start = NULL, *end = NULL;
	GdkDrawable *drawable;
	guint sr, er, sc, ec;
	guint i, j;

	start = tile->start_area;
	end = tile->end_area;

	sr = START_ROW(start->row, end->row);
	er = END_ROW(start->row, end->row);
	sc = START_COL(start->col, end->col);
	ec = END_COL(start->col, end->col);

	if(redraw)
		drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

	for(i = sr; i <= er; i++) {
		for(j = sc; j <= ec; j++) {
			if(tile->tarea[i][j].state & old) {
				tile->tarea[i][j].state &= ~old;
				tile->tarea[i][j].state |= new;
			}

			if(redraw)
				nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);
		}
	}
}

static void nftile_init(NFTILE *tile)
{
	GdkDrawable *drawable;
	gint line_gap = 0;
	guint left_space = 0;
	guint x, y, w, h;
	guint i, j;

	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

	if(!tile->tarea) {
		tile->tarea = imalloc((gsize)(sizeof(NFTileArea*) * tile->row_size));// * tile->col_size)));
		for(i = 0; i < tile->row_size; i++) {
			*(tile->tarea + i) = imalloc((gsize)(sizeof(NFTileArea) * (tile->col_size)));
		}
	}

	nfui_nfobject_get_offset((NFOBJECT*)tile, &x, &y);
	w = tile->object.width;
	h = tile->object.height;

	/* outline points */
	//line_gap = TILE_OUTLINE_W - 1;
	left_space = w % tile->col_size;

	tile->outPoints[0].x = x + line_gap + 1;
	tile->outPoints[0].y = y + line_gap + 1;

	tile->outPoints[1].x = x + w - line_gap - left_space;
	tile->outPoints[1].y = y + line_gap + 1;

	tile->outPoints[2].x = x + w - line_gap - left_space;
	tile->outPoints[2].y = y + h - line_gap - 1;

	tile->outPoints[3].x = x + line_gap + 1;
	tile->outPoints[3].y = y + h - line_gap - 1;

	tile->outPoints[4].x = x + line_gap + 1;
	tile->outPoints[4].y = y + line_gap + 1;

	w /= tile->col_size;
	h /= tile->row_size;

#if 0			// for debugging
printf(">>>>>>>\nobj x: %d obj y: %d\nobj width: %d obj height: %d \nrow: %d col: %d\nw/c: %d h/r: %d\n>>>>>>>\n",
			x, y,
			tile->object.width, tile->object.height,
			tile->row_size, tile->col_size,
			w, h);
#endif

	/* tile size */
	for(i=0; i<tile->row_size; i++) {
		for(j=0; j<tile->col_size; j++) {
			tile->tarea[i][j].row = i;
			tile->tarea[i][j].col = j;
			tile->tarea[i][j].x = (x + (w * j));
			tile->tarea[i][j].y = (y + (h * i));
			tile->tarea[i][j].width = w;
			tile->tarea[i][j].height = h;
			tile->tarea[i][j].select_color = SELECT_COLOR_START_INDEX;
			tile->tarea[i][j].preSelect_color = SELECT_COLOR_START_INDEX;

			if(!(tile->tarea[i][j].state & NFTILE_STATE_SELECT))
				tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;
		}
	}

	tile->start_area = tile->end_area = &tile->tarea[0][0];


	/* tile gc color */
	for(i=0; i<COLOR_COUNT; i++) {
		if(tile->filled) {
			if(!tile->bg_gc[i]) {
				tile->bg_gc[i] = gdk_gc_new(drawable);
				gdk_gc_set_rgb_fg_color(tile->bg_gc[i], &tile->color[i]);
			}
		}else {
			if(!tile->line_gc[i]) {
				tile->line_gc[i] = gdk_gc_new(drawable);
				gdk_gc_set_rgb_fg_color(tile->line_gc[i], &tile->color[i]);

				gdk_gc_set_line_attributes(tile->line_gc[i],
										TILE_LINE_NO_FILLED_W,
										GDK_LINE_SOLID,
										GDK_CAP_NOT_LAST,
										GDK_JOIN_MITER);
			}
		}
	}
}

static void nftile_draw_outlines(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *outline_gc;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	outline_gc = nfui_nfobject_get_gc(obj);

	if(obj->grab_kfocus) gdk_gc_set_rgb_fg_color(outline_gc, &UX_COLOR(137));
	else				 gdk_gc_set_rgb_fg_color(outline_gc, &UX_COLOR(136));
	gdk_gc_set_line_attributes(outline_gc,
			TILE_OUTLINE_W,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	gdk_draw_lines(drawable,
			outline_gc,
			NF_TILE(obj)->outPoints,
			5);

	nfui_nfobject_gc_unref(outline_gc);
}

static void nftile_draw_lines(NFTILE *tile)
{
	GdkDrawable *drawable;
	GdkColor line_color = UX_COLOR(731);
	GdkGC *border_gc;
	guint x, y, w, h;
	guint div_w, div_h;
	guint i;

	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);
	border_gc = nfui_nfobject_get_gc((NFOBJECT*)tile);
	gdk_gc_set_rgb_fg_color(border_gc, &line_color);

	nfui_nfobject_get_offset((NFOBJECT*)tile, &x, &y);
	w = tile->object.width;
	h = tile->object.height;

	div_w = (gint)w / tile->col_size;
	div_h = (gint)h / tile->row_size;

#if 0			// for debugging
	printf(">>>>>>>\nobj x: %d\nobj y: %d\nobj width: %d \nobj height: %d \n>>>>>>>%d %d\n",
			x, y,
			tile->object.width, tile->object.height,
			div_w, div_h);
#endif

	gdk_gc_set_line_attributes(border_gc,
							TILE_LINE_FILLED_W * 2,
							GDK_LINE_SOLID,
							GDK_CAP_NOT_LAST,
							GDK_JOIN_BEVEL);

	for(i=0; i<=tile->row_size; i++) {
		if(i == tile->row_size)
			gdk_draw_line(drawable,
					border_gc,
					(gint)x,
					(gint)(y - TILE_LINE_FILLED_W + (div_h * i)),
					(gint)(x + w - (w % tile->col_size)),
					(gint)(y - TILE_LINE_FILLED_W + (div_h * i)));
		else
			gdk_draw_line(drawable,
					border_gc,
					(gint)x,
					(gint)(y + (div_h * i)),
					(gint)(x + w - (w % tile->col_size)),
					(gint)(y + (div_h * i)));
	}

	for(i=0; i<=tile->col_size; i++) {
		if(i == tile->col_size)
			gdk_draw_line(drawable,
					border_gc,
					(gint)(x - TILE_LINE_FILLED_W + (div_w * i)),
					(gint)y,
					(gint)(x - TILE_LINE_FILLED_W + (div_w * i)),
					(gint)(y + h - (h % tile->row_size)));
		else
			gdk_draw_line(drawable,
					border_gc,
					(gint)(x + (div_w * i)),
					(gint)y,
					(gint)(x + (div_w * i)),
					(gint)(y + h - (h % tile->row_size)));
	}

	nfui_nfobject_gc_unref(border_gc);
}


static NFTileArea* nftile_get_area(NFTILE *tile, guint x, guint y)
{
    NFSCROLLEDFIXED *scrolled_fixed;
    gint scrolled_fixed_x, scrolled_fixed_y;
	guint tx, ty;
	guint w, h;
	guint ox, oy;
	guint row, col;

	nfui_nfobject_get_offset((NFOBJECT*)tile, &ox, &oy);

	if(x < ox || y < oy)
		return NULL;

	tx = (x - ox);
	ty = (y - oy);

    if (nfui_nfobject_is_scrolledfixed_usescr(tile) && nfui_nfobject_is_scrolledfixed_child(tile))
    {
        scrolled_fixed = nfui_nfobject_get_scrolledfixed(tile);
        nfui_nfobject_get_offset(scrolled_fixed, &scrolled_fixed_x, &scrolled_fixed_y);

        tx += (scrolled_fixed->relative_x - scrolled_fixed_x);
        ty += (scrolled_fixed->relative_y - scrolled_fixed_y);
    }    

	w = tile->object.width / tile->col_size;
	h = tile->object.height / tile->row_size;

/*
	if(tx < 0 || ty < 0)
		return NULL;
*/

	col = tx/w;
	row = ty/h;

	if(tile->row_size <= row || tile->col_size <= col)
		return NULL;

	return &tile->tarea[row][col];
}


static void nftile_key_move(NFTILE *tile, GdkDrawable *drawable, KEYPAD_KID dir)
{
	gint row, col;
	guint i, j;
	guint sr, er, sc, ec;

	if(tile->pushed_enter) {
		row = tile->end_area->row;
		col = tile->end_area->col;
	}else {
		row = tile->start_area->row;
		col = tile->start_area->col;
	}

	if(dir == KEYPAD_UP)
	{
		if(row - 1 < 0)	return;
	}
	else if(dir == KEYPAD_DOWN)
	{
		if((guint)row + 1 > tile->row_size - 1) return;
	}
	else if(dir == KEYPAD_LEFT)
	{
		if(col - 1 < 0)	return;
	}
	else if(dir == KEYPAD_RIGHT)
	{
		if((guint)col + 1 > tile->col_size - 1) return;
	}

	/* previous state */
	if(tile->pushed_enter)
	{
		sr = START_ROW(tile->start_area->row, tile->end_area->row);
		er = END_ROW(tile->start_area->row, tile->end_area->row);
		sc = START_COL(tile->start_area->col, tile->end_area->col);
		ec = END_COL(tile->start_area->col, tile->end_area->col);

		for(i = sr; i <= er; i++) {
			if(tile->tarea[i][col].state & NFTILE_STATE_FOCUS) {
				tile->tarea[i][col].state &= ~NFTILE_STATE_FOCUS;
				tile->tarea[i][col].state |= NFTILE_STATE_NORMAL;

				nftile_draw_color(tile, &tile->tarea[i][col], drawable, tile->tarea[i][col].state, FALSE);
			}else
				nftile_draw_color(tile, &tile->tarea[i][col], drawable, tile->tarea[i][col].state, FALSE);
		}

		for(j = sc; j <= ec; j++) {
			if(tile->tarea[row][j].state & NFTILE_STATE_FOCUS) {
				tile->tarea[row][j].state &= ~NFTILE_STATE_FOCUS;
				tile->tarea[row][j].state |= NFTILE_STATE_NORMAL;

				nftile_draw_color(tile, &tile->tarea[row][j], drawable, tile->tarea[row][j].state, FALSE);
			}else
				nftile_draw_color(tile, &tile->tarea[row][j], drawable, tile->tarea[row][j].state, FALSE);
		}
	}
	else
	{
		if(tile->tarea[row][col].state & NFTILE_STATE_FOCUS) {
			tile->tarea[row][col].state &= ~NFTILE_STATE_FOCUS;
			tile->tarea[row][col].state |= NFTILE_STATE_NORMAL;
		}

		if(tile->tarea[row][col].state > NFTILE_STATE_NONE)
			nftile_draw_color(tile, &tile->tarea[row][col], drawable, tile->tarea[row][col].state, FALSE);

	}

	/* new state */
	if(dir == KEYPAD_UP)
		row -= 1;
	else if(dir == KEYPAD_DOWN)
		row += 1;
	else if(dir == KEYPAD_LEFT)
		col -= 1;
	else if(dir == KEYPAD_RIGHT)
		col += 1;

	if(tile->pushed_enter)
	{
		tile->end_area = &tile->tarea[row][col];

		sr = START_ROW(tile->start_area->row, tile->end_area->row);
		er = END_ROW(tile->start_area->row, tile->end_area->row);
		sc = START_COL(tile->start_area->col, tile->end_area->col);
		ec = END_COL(tile->start_area->col, tile->end_area->col);

		for(i = sr; i <= er; i++) {
			for(j = sc; j <= ec; j++) {
				if(tile->tarea[i][j].state < NFTILE_STATE_SELECT) {
					tile->tarea[i][j].state &= NFTILE_STATE_NONE;
					tile->tarea[i][j].state |= NFTILE_STATE_FOCUS;

					nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);

					nfui_user_signal_emit((NFOBJECT*)tile, NFEVENT_TILE_MOVE_SELECT, FALSE);
				}else {
					nftile_draw_color(tile, &tile->tarea[i][j], drawable, NFTILE_STATE_FOCUS, FALSE);

					nfui_user_signal_emit((NFOBJECT*)tile, NFEVENT_TILE_MOVE_SELECT, FALSE);
				}
			}
		}
	}
	else
	{
		if(tile->tarea[row][col].state & NFTILE_STATE_NORMAL) {
			tile->tarea[row][col].state &= ~NFTILE_STATE_NORMAL;
			tile->tarea[row][col].state |= NFTILE_STATE_FOCUS;

			nftile_draw_color(tile, &tile->tarea[row][col], drawable, tile->tarea[row][col].state, FALSE);
		}else if(tile->tarea[row][col].state & NFTILE_STATE_SELECT
				|| tile->tarea[row][col].state & NFTILE_STATE_RESELECT) {
			nftile_draw_color(tile, &tile->tarea[row][col], drawable, NFTILE_STATE_FOCUS, FALSE);
		}

		tile->start_area = tile->end_area = &tile->tarea[row][col];
	}

	if(tile->draw_outline && tile->object.kfocus == NFOBJECT_FOCUS)
		nftile_draw_outlines((NFOBJECT*)tile);
}


static void nftile_draw_color(NFTILE *tile, NFTileArea *area, GdkDrawable *drawable, NFTileState state, gboolean force)
{
	static NFTileState area_ac[60][60] = {NFTILE_STATE_NONE, };


	if(NFTILE_STATE_NORMAL > state || NFTILE_STATE_RESELECT < state)
		printf(">>>>row:%d, col:%d, draw color : %d \n", area->row, area->col, state);

	g_return_if_fail(NFTILE_STATE_NORMAL <= state && NFTILE_STATE_RESELECT >= state);


	if (force)
	{
		area_ac[area->row][area->col] = state;
	}
	else
	{
		if (area_ac[area->row][area->col] == state)
			return;

		area_ac[area->row][area->col] = state;
	}

	if(tile->filled) {
		switch(state) {
			case NFTILE_STATE_NONE:
				break;
			case NFTILE_STATE_NORMAL:
				gdk_draw_rectangle(drawable,
						tile->bg_gc[0],
						TRUE,
						(gint)area->x + TILE_LINE_FILLED_W,
						(gint)area->y + TILE_LINE_FILLED_W,
						(gint)area->width - (TILE_LINE_FILLED_W * 2),
						(gint)area->height - (TILE_LINE_FILLED_W * 2));
				break;
			case NFTILE_STATE_FOCUS:
				gdk_draw_rectangle(drawable,
						tile->bg_gc[1],
						TRUE,
						(gint)area->x + TILE_LINE_FILLED_W,
						(gint)area->y + TILE_LINE_FILLED_W,
						(gint)area->width - (TILE_LINE_FILLED_W * 2),
						(gint)area->height - (TILE_LINE_FILLED_W * 2));
				break;
			case NFTILE_STATE_RESELECT:
			case NFTILE_STATE_SELECT:
				{
					GdkGC *gc;
					gint color_idx = area->select_color;

					if(area->select_color <= SELECT_STATE_COLOR_4)
					{
						gdk_draw_rectangle(drawable,
								tile->bg_gc[area->select_color],
								TRUE,
								(gint)area->x + TILE_LINE_FILLED_W,
								(gint)area->y + TILE_LINE_FILLED_W,
								(gint)area->width - (TILE_LINE_FILLED_W * 2),
								(gint)area->height - (TILE_LINE_FILLED_W * 2));
					}
					else
					{
						gc = gdk_gc_new(drawable);
        				gdk_gc_set_rgb_fg_color(gc, &ux_rtile_colors[color_idx]);
						gdk_draw_rectangle(drawable,
								gc,
								TRUE,
								(gint)area->x + TILE_LINE_FILLED_W,
								(gint)area->y + TILE_LINE_FILLED_W,
								(gint)area->width - (TILE_LINE_FILLED_W * 2),
								(gint)area->height - (TILE_LINE_FILLED_W * 2));
						g_object_unref(gc);
					}
				}
				break;
		}
	}else {
		GdkPoint points[5];
		gint gap;
		gint i;

		gap = TILE_LINE_NO_FILLED_W/2;

		points[0].x = area->x + gap;
		points[0].y = area->y + gap;
		points[1].x = area->x + area->width - gap;
		points[1].y = area->y + gap;
		points[2].x = area->x + area->width - gap;
		points[2].y = area->y + area->height - gap;
		points[3].x = area->x + gap;
		points[3].y = area->y + area->height - gap;
		points[4].x = area->x + gap;
		points[4].y = area->y + gap;


		switch(state) {
			case NFTILE_STATE_NONE:
				break;
			case NFTILE_STATE_NORMAL:
				// clear area
				gdk_draw_rectangle(drawable,
						tile->line_gc[0],
						TRUE,
						(gint)area->x + TILE_LINE_NO_FILLED_W,
						(gint)area->y + TILE_LINE_NO_FILLED_W,
						(gint)area->width - (TILE_LINE_NO_FILLED_W * 2),
						(gint)area->height - (TILE_LINE_NO_FILLED_W * 2));

				gdk_draw_lines(drawable,
						tile->line_gc[0],
						points,
						5);
				break;
			case NFTILE_STATE_FOCUS:
				gdk_draw_lines(drawable,
						tile->line_gc[1],
						points,
						5);
				break;
			case NFTILE_STATE_RESELECT:
			case NFTILE_STATE_SELECT:
				gdk_draw_rectangle(drawable,
						tile->line_gc[area->select_color],
						FALSE,
						(gint)area->x + (TILE_LINE_NO_FILLED_W * 2),
						(gint)area->y + (TILE_LINE_NO_FILLED_W * 2),
						(gint)area->width - (TILE_LINE_NO_FILLED_W * 4),
						(gint)area->height - (TILE_LINE_NO_FILLED_W * 4));

				gdk_draw_lines(drawable,
						tile->line_gc[0],
						points,
						5);
				break;
		}
	}
}


NFOBJECT* nfui_tile_new(guint row, guint col)
{
	NFTILE *tile;

	GdkColor default_color[COLOR_COUNT] = {UX_COLOR(732),
											UX_COLOR(733),
											UX_COLOR(734),
											UX_COLOR(734),
											UX_COLOR(734),
											UX_COLOR(734)};

	tile = (NFTILE*)imalloc(sizeof(NFTILE));
	if(!tile) {
		DMSG(1, "NFTile alloc error...");
		return NULL;
	}

#if 0
	tile->object.parent = NULL;
	tile->object.x = 0;
	tile->object.y = 0;
	tile->object.width = 200;
	tile->object.height = 100;
	tile->object.type = NFOBJECT_TYPE_NFTILE;
	tile->object.show = NFOBJECT_HIDE;
	tile->object.use_focus = NFOBJECT_FOCUS_ON;
	tile->object.kfocus = NFOBJECT_UNFOCUS;
	tile->object.mfocus = NFOBJECT_UNFOCUS;
	tile->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	tile->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	tile->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	tile->object.pre_event_handler = NULL;
	tile->object.default_event_handler = nftile_event_handler;
	tile->object.post_event_handler = NULL;
	tile->object.grab_kfocus = FALSE;
#else
	nfui_nfobject_init((NFOBJECT*)tile);

	tile->object.width = 200;
	tile->object.height = 100;
	tile->object.type = NFOBJECT_TYPE_NFTILE;
	tile->object.use_focus = NFOBJECT_FOCUS_ON;
	tile->object.default_event_handler = nftile_event_handler;
#endif

	memcpy(tile->color, default_color, sizeof(default_color));

	tile->row_size = row;
	tile->col_size = col;
	tile->line_border = 4;
	tile->select_n = SELECT_COLOR_START_INDEX;
	tile->tarea = NULL;
	tile->start_area = NULL;
	tile->end_area = NULL;
	tile->sensitive = TRUE;
	tile->pressed = FALSE;
	tile->can_reselect = FALSE;
	tile->filled = TRUE;
	tile->draw_outline = TRUE;
	tile->innout = 0;

	/* For key navi */
	tile->pushed_enter = FALSE;

	return (NFOBJECT*)tile;
}



void nfui_tile_set_selectArea(NFTILE *tile, guint row1, guint col1, guint row2, guint col2)
{
	GdkDrawable *drawable = NULL;
	guint i, j;


	g_return_if_fail(tile != NULL);
	g_return_if_fail(tile->row_size > row1 || tile->row_size > row2);
	g_return_if_fail(tile->col_size > col1 || tile->col_size > col2);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}


	if(!tile->tarea) {
		tile->tarea = imalloc((gsize)(sizeof(NFTileArea*) * tile->row_size));// * tile->col_size)));
		for(i = 0; i < tile->row_size; i++) {
			*(tile->tarea + i) = imalloc((gsize)(sizeof(NFTileArea) * (tile->col_size)));
		}

	}

	for(i = row1; i < row2 + 1; i++) {
		for(j = col1; j < col2 + 1; j++) {
			if(tile->tarea[i][j].state & NFTILE_STATE_SELECT) {
				tile->tarea[i][j].select_color = tile->select_n;
			}else {
				tile->tarea[i][j].state = 0;
				tile->tarea[i][j].state |= NFTILE_STATE_SELECT;

				tile->tarea[i][j].select_color = tile->select_n;
			}

			if(((NFOBJECT*)tile)->parent && ((NFOBJECT*)tile)->show) {
				if(!drawable)
					drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

				if(drawable) {
					nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);
				}
			}
		}
	}

	if(COLOR_COUNT <= ++tile->select_n) {
		tile->select_n = SELECT_COLOR_START_INDEX;
	}
}


void nfui_tile_conv_selectArea(NFTILE *tile, guint row1, guint col1, guint row2, guint col2)
{
	GdkDrawable *drawable = NULL;
	guint i, j;


	g_return_if_fail(tile != NULL);
	g_return_if_fail(tile->row_size > row1 || tile->row_size > row2);
	g_return_if_fail(tile->col_size > col1 || tile->col_size > col2);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	for(i = row1; i < row2 + 1; i++) {
		for(j = col1; j < col2 + 1; j++) {
			if(tile->tarea[i][j].state & NFTILE_STATE_SELECT) {
				tile->tarea[i][j].state &= ~NFTILE_STATE_SELECT;
				tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;

				if(tile->select_n != tile->tarea[i][j].select_color)
					--tile->select_n;

			}else if(tile->tarea[i][j].state & NFTILE_STATE_RESELECT) {
				tile->tarea[i][j].state &= ~NFTILE_STATE_RESELECT;
				tile->tarea[i][j].state |= NFTILE_STATE_SELECT;

				if(tile->select_n - 1 == tile->tarea[i][j].select_color)
					--tile->select_n;

				tile->tarea[i][j].select_color = tile->tarea[i][j].preSelect_color;
			}else if(tile->tarea[i][j].state & NFTILE_STATE_FOCUS) {
				tile->tarea[i][j].state &= ~NFTILE_STATE_FOCUS;
				tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;

				if(tile->select_n != tile->tarea[i][j].select_color)
					--tile->select_n;
			}

			if(tile->select_n < SELECT_COLOR_START_INDEX)
				tile->select_n = SELECT_COLOR_START_INDEX;

			drawable = nfui_nfobject_get_window((NFOBJECT*)tile);
			if(drawable)
				nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);
		}
	}
}


void nfui_tile_get_selectArea(NFTILE *tile, guint *row1, guint *col1, guint *row2, guint *col2)
{
	g_return_if_fail(tile != NULL);
	g_return_if_fail(row1 != NULL || row2 != NULL);
	g_return_if_fail(col1 != NULL || col2 != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	if(tile->pressed) {
		if(tile->start_area == tile->end_area) {
			*row1 = START_ROW(tile->start_area->row, tile->end_area->row);
			*row2 = END_ROW(tile->start_area->row, tile->end_area->row);
			*col1 = START_COL(tile->start_area->col, tile->end_area->col);
			*col2 = END_COL(tile->start_area->col, tile->end_area->col);
		}

	}else {
		*row1 = START_ROW(tile->start_area->row, tile->end_area->row);
		*row2 = END_ROW(tile->start_area->row, tile->end_area->row);
		*col1 = START_COL(tile->start_area->col, tile->end_area->col);
		*col2 = END_COL(tile->start_area->col, tile->end_area->col);
	}
}


void nfui_tile_set_selectRect(NFTILE *tile, GdkRectangle rect)
{
	GdkDrawable *drawable = NULL;
	guint sr, sc, er, ec;
	guint i, j;

	g_return_if_fail(tile != NULL);
	g_return_if_fail(tile->tarea != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	sr = (rect.x/tile->tarea[0][0].width);
	sc = (rect.y/tile->tarea[0][0].height);
	er = (rect.x+rect.width)/tile->tarea[0][0].width;
	ec = (rect.y+rect.height)/tile->tarea[0][0].height;

	for(i=sr; i<=er; i++) {
		for(j=sc; j<=ec; j++) {
			tile->tarea[i][j].state = 0;
			tile->tarea[i][j].state |= NFTILE_STATE_SELECT;

			tile->tarea[i][j].select_color = tile->select_n;

			if(((NFOBJECT*)tile)->parent && ((NFOBJECT*)tile)->show) {
				if(!drawable)
					drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

				if(drawable)
					nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);
			}
		}
	}

	if(COLOR_COUNT <= ++tile->select_n) {
		tile->select_n = SELECT_COLOR_START_INDEX;
	}
}


void nfui_tile_get_selectRect(NFTILE *tile, GdkRectangle *rect)
{
	gint sr, sc, er, ec;

	g_return_if_fail(tile != NULL);
	g_return_if_fail(tile->tarea != NULL);
	g_return_if_fail(rect != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	sr = START_ROW(tile->start_area->row, tile->end_area->row);
	er = END_ROW(tile->start_area->row, tile->end_area->row);
	sc = START_COL(tile->start_area->col, tile->end_area->col);
	ec = END_COL(tile->start_area->col, tile->end_area->col);

	rect->x = tile->tarea[sr][sc].x;
	rect->y = tile->tarea[sr][sc].y;
	rect->width = tile->tarea[er][ec].x - tile->tarea[sr][sc].x;
	rect->height = tile->tarea[er][ec].y - tile->tarea[sr][sc].y;
}


void nfui_tile_set_color(NFTILE *tile, NFTileState state, GdkColor *state_color)
{
	GdkDrawable *drawable = NULL;

	g_return_if_fail(tile != NULL);
	g_return_if_fail(NFTILE_STATE_NORMAL <= state && NFTILE_STATE_RESELECT >= state);
	g_return_if_fail(state_color != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

	switch(state) {
		case NFTILE_STATE_NONE:
			break;
		case NFTILE_STATE_NORMAL:
			{
				tile->color[NORMAL_STATE_COLOR].red = state_color->red;
				tile->color[NORMAL_STATE_COLOR].green = state_color->green;
				tile->color[NORMAL_STATE_COLOR].blue = state_color->blue;

				if(drawable) {
					if(tile->bg_gc[NORMAL_STATE_COLOR]) {
						g_object_unref(tile->bg_gc[NORMAL_STATE_COLOR]);
						tile->bg_gc[NORMAL_STATE_COLOR] = NULL;
					}

					tile->bg_gc[NORMAL_STATE_COLOR] = gdk_gc_new(drawable);
					gdk_gc_set_rgb_fg_color(tile->bg_gc[NORMAL_STATE_COLOR], &tile->color[NORMAL_STATE_COLOR]);
				}
			}
			break;
		case NFTILE_STATE_FOCUS:
			{
				tile->color[FOCUS_STATE_COLOR].red = state_color->red;
				tile->color[FOCUS_STATE_COLOR].green = state_color->green;
				tile->color[FOCUS_STATE_COLOR].blue = state_color->blue;

				if(drawable) {
					if(tile->bg_gc[FOCUS_STATE_COLOR]) {
						g_object_unref(tile->bg_gc[FOCUS_STATE_COLOR]);
						tile->bg_gc[FOCUS_STATE_COLOR] = NULL;
					}

					tile->bg_gc[FOCUS_STATE_COLOR] = gdk_gc_new(drawable);
					gdk_gc_set_rgb_fg_color(tile->bg_gc[FOCUS_STATE_COLOR], &tile->color[FOCUS_STATE_COLOR]);
				}
			}
			break;
		case NFTILE_STATE_RESELECT:
		case NFTILE_STATE_SELECT:
			{
				gint i = 0, j = 0;

				for(i=SELECT_STATE_COLOR_1; i<=SELECT_STATE_COLOR_4; i++, j++) {
					tile->color[i].red = state_color[j].red;
					tile->color[i].green = state_color[j].green;
					tile->color[i].blue = state_color[j].blue;

					if(drawable) {
						if(tile->bg_gc[i]) {
							g_object_unref(tile->bg_gc[i]);
							tile->bg_gc[i] = NULL;
						}

						tile->bg_gc[i] = gdk_gc_new(drawable);
						gdk_gc_set_rgb_fg_color(tile->bg_gc[i], &tile->color[i]);
					}
				}
			}
			break;
	}
}

void nfui_tile_set_line_color(NFTILE *tile, NFTileState state, GdkColor *state_color)
{
	GdkDrawable *drawable = NULL;

	g_return_if_fail(tile != NULL);
	g_return_if_fail(NFTILE_STATE_NORMAL <= state && NFTILE_STATE_RESELECT >= state);
	g_return_if_fail(state_color != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

	switch(state) {
		case NFTILE_STATE_NONE:
			break;
		case NFTILE_STATE_NORMAL:
			{
				tile->color[NORMAL_STATE_COLOR].red = state_color->red;
				tile->color[NORMAL_STATE_COLOR].green = state_color->green;
				tile->color[NORMAL_STATE_COLOR].blue = state_color->blue;

				if(drawable) {
					if(tile->line_gc[NORMAL_STATE_COLOR]) {
						g_object_unref(tile->line_gc[NORMAL_STATE_COLOR]);
						tile->line_gc[NORMAL_STATE_COLOR] = NULL;
					}

					tile->line_gc[NORMAL_STATE_COLOR] = gdk_gc_new(drawable);
					gdk_gc_set_rgb_fg_color(tile->line_gc[NORMAL_STATE_COLOR], &tile->color[NORMAL_STATE_COLOR]);
				}
			}
			break;
		case NFTILE_STATE_FOCUS:
			{
				tile->color[FOCUS_STATE_COLOR].red = state_color->red;
				tile->color[FOCUS_STATE_COLOR].green = state_color->green;
				tile->color[FOCUS_STATE_COLOR].blue = state_color->blue;

				if(drawable) {
					if(tile->line_gc[FOCUS_STATE_COLOR]) {
						g_object_unref(tile->line_gc[FOCUS_STATE_COLOR]);
						tile->line_gc[FOCUS_STATE_COLOR] = NULL;
					}

					tile->line_gc[FOCUS_STATE_COLOR] = gdk_gc_new(drawable);
					gdk_gc_set_rgb_fg_color(tile->line_gc[FOCUS_STATE_COLOR], &tile->color[FOCUS_STATE_COLOR]);
				}
			}
			break;
		case NFTILE_STATE_RESELECT:
		case NFTILE_STATE_SELECT:
			{
				gint i, j = 0;

				for(i=SELECT_STATE_COLOR_1; i<=SELECT_STATE_COLOR_4; i++, j++) {
					tile->color[i].red = state_color[j].red;
					tile->color[i].green = state_color[j].green;
					tile->color[i].blue = state_color[j].blue;

					if(drawable) {
						if(tile->line_gc[i]) {
							g_object_unref(tile->line_gc[i]);
							tile->line_gc[i] = NULL;
						}

						tile->line_gc[i] = gdk_gc_new(drawable);
						gdk_gc_set_rgb_fg_color(tile->line_gc[i], &tile->color[i]);
					}
				}
			}
			break;
	}


}


void nfui_tile_draw_color(NFTILE *tile, guint color_n, guint row1, guint col1, guint row2, guint col2)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	guint color_idx = color_n;
	guint i, j;

	g_return_if_fail(tile != NULL);
	g_return_if_fail(color_n <= SELECT_STATE_EX_COLOR_24);
	g_return_if_fail(tile->row_size > row1 || tile->row_size > row2);
	g_return_if_fail(tile->col_size > col1 || tile->col_size > col2);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}


	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);
	if(!drawable) {
		DMSG(1, "GdkDrawable is NULL..");
		return;
	}

	if(!tile->tarea) {
		DMSG(1, "tile->tarea is NULL..");
		return;
	}

	for(i = row1; i < row2 + 1; i++) {
		for(j = col1; j < col2 + 1; j++) {

			switch(color_n) {
				case NORMAL_STATE_COLOR:
					tile->tarea[i][j].state = 0;
					tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;
					break;
				case FOCUS_STATE_COLOR:
					break;
				case SELECT_STATE_COLOR_1:
				case SELECT_STATE_COLOR_2:
				case SELECT_STATE_COLOR_3:
				case SELECT_STATE_COLOR_4:
					{
						if(tile->tarea[i][j].state & NFTILE_STATE_SELECT)
						{
							tile->tarea[i][j].state = 0;

							if(tile->can_reselect)
								tile->tarea[i][j].state |= NFTILE_STATE_RESELECT;
							else
								tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
						}
						else
						{
							tile->tarea[i][j].state = 0;
							tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
						}

						tile->tarea[i][j].select_color = color_n;
					}
					break;
				default:
					{
						if(color_n <= SELECT_STATE_EX_COLOR_24)
						{
							if(tile->tarea[i][j].state & NFTILE_STATE_SELECT)
							{
								tile->tarea[i][j].state = 0;

								if(tile->can_reselect)
									tile->tarea[i][j].state |= NFTILE_STATE_RESELECT;
								else
									tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
							}
							else
							{
								tile->tarea[i][j].state = 0;
								tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
							}

							tile->tarea[i][j].select_color = color_n;
						}
					}
					break;
			}

			if(color_n <= SELECT_STATE_COLOR_4) {
				gdk_draw_rectangle(drawable,
						tile->bg_gc[color_n],
						TRUE,
						(gint)tile->tarea[i][j].x + TILE_LINE_FILLED_W,
						(gint)tile->tarea[i][j].y + TILE_LINE_FILLED_W,
						(gint)tile->tarea[i][j].width - (TILE_LINE_FILLED_W * 2),
						(gint)tile->tarea[i][j].height - (TILE_LINE_FILLED_W * 2));
			}else {
				gc = gdk_gc_new(drawable);
				gdk_gc_set_rgb_fg_color(gc, &ux_rtile_colors[color_idx]);
				gdk_draw_rectangle(drawable,
						gc,
						TRUE,
						(gint)tile->tarea[i][j].x + TILE_LINE_FILLED_W,
						(gint)tile->tarea[i][j].y + TILE_LINE_FILLED_W,
						(gint)tile->tarea[i][j].width - (TILE_LINE_FILLED_W * 2),
						(gint)tile->tarea[i][j].height - (TILE_LINE_FILLED_W * 2));
				g_object_unref(gc);
			}
		}
	}

}

void nfui_tile_no_draw_color(NFTILE *tile, guint color_n, guint row1, guint col1, guint row2, guint col2)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
	guint color_idx = color_n;
	guint i, j;

	g_return_if_fail(tile != NULL);
	g_return_if_fail(color_n <= SELECT_STATE_EX_COLOR_24);
	g_return_if_fail(tile->row_size > row1 || tile->row_size > row2);
	g_return_if_fail(tile->col_size > col1 || tile->col_size > col2);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}


	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);
	if(!drawable) {
		DMSG(1, "GdkDrawable is NULL..");
		return;
	}

	if(!tile->tarea) {
		DMSG(1, "tile->tarea is NULL..");
		return;
	}

	for(i = row1; i < row2 + 1; i++) {
		for(j = col1; j < col2 + 1; j++) {

			switch(color_n) {
				case NORMAL_STATE_COLOR:
					tile->tarea[i][j].state = 0;
					tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;
					break;
				case FOCUS_STATE_COLOR:
					break;
				case SELECT_STATE_COLOR_1:
				case SELECT_STATE_COLOR_2:
				case SELECT_STATE_COLOR_3:
				case SELECT_STATE_COLOR_4:
					{
						if(tile->tarea[i][j].state & NFTILE_STATE_SELECT)
						{
							tile->tarea[i][j].state = 0;

							if(tile->can_reselect)
								tile->tarea[i][j].state |= NFTILE_STATE_RESELECT;
							else
								tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
						}
						else
						{
							tile->tarea[i][j].state = 0;
							tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
						}

						tile->tarea[i][j].select_color = color_n;
					}
					break;
				default:
					{
						if(color_n <= SELECT_STATE_EX_COLOR_24)
						{
							if(tile->tarea[i][j].state & NFTILE_STATE_SELECT)
							{
								tile->tarea[i][j].state = 0;

								if(tile->can_reselect)
									tile->tarea[i][j].state |= NFTILE_STATE_RESELECT;
								else
									tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
							}
							else
							{
								tile->tarea[i][j].state = 0;
								tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
							}

							tile->tarea[i][j].select_color = color_n;
						}
					}
					break;
			}
		}
	}

}

void nfui_tile_sensitive(NFTILE *tile, gboolean sensitive)
{
	g_return_if_fail(tile != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	if(tile->sensitive != sensitive)
		tile->sensitive = sensitive;
}


guint nfui_tile_get_cur_state(NFTILE *tile)
{
	g_return_val_if_fail(tile != NULL, 0);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return 0;
	}

	return tile->end_area->state;
}


void nfui_tile_can_reSelect(NFTILE *tile, gboolean reselect)
{
	g_return_if_fail(tile != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	tile->can_reselect = reselect;
}

gint nfui_tile_get_state(NFTILE *tile, guint row, guint col)
{
	g_return_val_if_fail(tile != NULL, -1);
	g_return_val_if_fail(tile->row_size > row || tile->col_size > col, -1);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return -1;
	}

	if(!tile->tarea)
		return -1;

	return tile->tarea[row][col].state;
}


gboolean nfui_tile_drawable(NFTILE *tile)
{
	g_return_val_if_fail(tile != NULL, -1);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return FALSE;
	}

	if(tile->tarea)
		return TRUE;

	return FALSE;
}

void nfui_tile_set_fill(NFTILE *tile, gboolean filled)
{
	g_return_if_fail(tile != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	tile->filled = filled;
}

void nfui_tile_get_start_area(NFTILE *tile, guint *row, guint *col)
{
	g_return_if_fail(tile != NULL);
	g_return_if_fail(row != NULL);
	g_return_if_fail(col != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	*row = tile->start_area->row;
	*col = tile->start_area->col;
}

void nfui_tile_get_end_area(NFTILE *tile, guint *row, guint *col)
{
	g_return_if_fail(tile != NULL);
	g_return_if_fail(row != NULL);
	g_return_if_fail(col != NULL);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	*row = tile->end_area->row;
	*col = tile->end_area->col;
}

void nfui_tile_set_drawable_outline(NFTILE *tile, gboolean draw_outline)
{
	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	if(tile->draw_outline != draw_outline)
		tile->draw_outline = draw_outline;

}

void nfui_tile_set_area_color(NFTILE *tile, guint color_n, guint row1, guint col1, guint row2, guint col2)
{
	guint i, j;

	g_return_if_fail(tile != NULL);
	g_return_if_fail(color_n <= SELECT_STATE_EX_COLOR_24);
	g_return_if_fail(tile->row_size > row1 || tile->row_size > row2);
	g_return_if_fail(tile->col_size > col1 || tile->col_size > col2);

	if(tile->object.type != NFOBJECT_TYPE_NFTILE)
	{
		return;
	}

	if(!tile->tarea) {
		DMSG(1, "tile->tarea is NULL..");
		return;
	}

	for(i = row1; i < row2 + 1; i++) {
		for(j = col1; j < col2 + 1; j++) {

			switch(color_n) {
				case NORMAL_STATE_COLOR:
					tile->tarea[i][j].state = 0;
					tile->tarea[i][j].state |= NFTILE_STATE_NORMAL;
					break;
				case FOCUS_STATE_COLOR:
					break;
				case SELECT_STATE_COLOR_1:
				case SELECT_STATE_COLOR_2:
				case SELECT_STATE_COLOR_3:
				case SELECT_STATE_COLOR_4:
					{
						if(tile->tarea[i][j].state & NFTILE_STATE_SELECT) {
							tile->tarea[i][j].state = 0;

							if(tile->can_reselect) {
								tile->tarea[i][j].state |= NFTILE_STATE_RESELECT;
							}else {
								tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
							}
						}else {
							tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
						}

						tile->tarea[i][j].select_color = color_n;
					}
					break;
				default:
					{
						if(color_n <= SELECT_STATE_EX_COLOR_24) {
							if(tile->tarea[i][j].state & NFTILE_STATE_SELECT) {
								tile->tarea[i][j].state = 0;

								if(tile->can_reselect) {
									tile->tarea[i][j].state |= NFTILE_STATE_RESELECT;
								}else {
									tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
								}
							}else {
								tile->tarea[i][j].state |= NFTILE_STATE_SELECT;
							}

							tile->tarea[i][j].select_color = color_n;
						}
					}
					break;
			}
		}
	}

}

gint nfui_tile_reset_area_size(NFTILE *tile, guint rows, guint cols)
{
    gint i, j;
    gint x, y, w, h;

    gint init = 0;

	if (tile->object.type != NFOBJECT_TYPE_NFTILE)
		return -1;

	if ((tile->row_size == rows) && (tile->col_size == cols))
		return -1;

	if(tile->tarea) {
		for(i = 0; i < tile->row_size; i++) {
			ifree(*(tile->tarea + i));
		}

		ifree(tile->tarea);
		tile->tarea = NULL;

		init = 1;
	}

	for(i=0; i<COLOR_COUNT; i++) {
		if(tile->filled) {
			if(tile->bg_gc[i]) {
				g_object_unref(tile->bg_gc[i]);
				tile->bg_gc[i] = NULL;
			}
		}else {
			if(tile->line_gc[i]) {
				g_object_unref(tile->line_gc[i]);
				tile->line_gc[i] = NULL;
			}
		}
	}

	tile->row_size = rows;
	tile->col_size = cols;

	if (init) nftile_init(tile);

    return 0;
}

void nfui_tile_set_line_border(NFTILE *tile, gint border)
{
	if (tile->object.type != NFOBJECT_TYPE_NFTILE)
		return -1;

	tile->line_border = border;
}


gboolean nfui_tile_set_select_state(NFTILE *tile, guint row, guint col)
{
	g_return_val_if_fail(tile->object.type == NFOBJECT_TYPE_NFTILE, FALSE);
	g_return_val_if_fail(tile->row_size > row && tile->col_size > col, FALSE);

	if(!tile->tarea) {
		DMSG(1, "tile->tarea is NULL..");
		return FALSE;
	}

	if(tile->tarea[row][col].state & NFTILE_STATE_SELECT) {
		tile->tarea[row][col].select_color = tile->select_n;
	}else {
		tile->tarea[row][col].state = 0;
		tile->tarea[row][col].state |= NFTILE_STATE_SELECT;

		tile->tarea[row][col].select_color = tile->select_n;
	}

	if(COLOR_COUNT <= ++tile->select_n)
		tile->select_n = SELECT_COLOR_START_INDEX;

	return TRUE;
}

gboolean nfui_tile_set_conv_select_state(NFTILE *tile, guint row, guint col)
{
	g_return_val_if_fail(tile->object.type == NFOBJECT_TYPE_NFTILE, FALSE);
	g_return_val_if_fail(tile->row_size > row && tile->col_size > col, FALSE);

	if(!tile->tarea) {
		DMSG(1, "tile->tarea is NULL..");
		return FALSE;
	}

	if(tile->tarea[row][col].state & NFTILE_STATE_SELECT) {
		tile->tarea[row][col].state &= ~NFTILE_STATE_SELECT;
		tile->tarea[row][col].state |= NFTILE_STATE_NORMAL;

		if(tile->select_n != tile->tarea[row][col].select_color)
			--tile->select_n;

	}else if(tile->tarea[row][col].state & NFTILE_STATE_RESELECT) {
		tile->tarea[row][col].state &= ~NFTILE_STATE_RESELECT;
		tile->tarea[row][col].state |= NFTILE_STATE_SELECT;

		if(tile->select_n - 1 == tile->tarea[row][col].select_color)
			--tile->select_n;

		tile->tarea[row][col].select_color = tile->tarea[row][col].preSelect_color;
	}

	if(tile->select_n < SELECT_COLOR_START_INDEX)
		tile->select_n = SELECT_COLOR_START_INDEX;

	return TRUE;
}

void nfui_tile_draw_area(NFTILE *tile, guint row1, guint col1, guint row2, guint col2)
{
	GdkDrawable *drawable = NULL;
	guint i, j;

	g_return_if_fail(tile->object.type == NFOBJECT_TYPE_NFTILE);
	g_return_if_fail(tile->row_size > row1 || tile->row_size > row2);
	g_return_if_fail(tile->col_size > col1 || tile->col_size > col2);


	drawable = nfui_nfobject_get_window((NFOBJECT*)tile);

	for(i = row1; i < row2 + 1; i++) {
		for(j = col1; j < col2 + 1; j++) {
			nftile_draw_color(tile, &tile->tarea[i][j], drawable, tile->tarea[i][j].state, FALSE);
		}
	}

}
