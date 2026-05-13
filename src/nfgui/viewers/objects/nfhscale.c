
#include "../../support/util.h"
#include "../../support/event_loop.h"
#include "../../support/color.h"


#include "nfhscale.h"
#include "ix_mem.h"


#define HSCALE_MIN_VAL 					0
#define HSCALE_MAX_VAL 					9
#define START_X_GAP						5

#define HSCALE_LINE_BORDER				2



static gboolean nfhscale_event_handler(NFHSCALE *hscale, GdkEvent *event, gpointer data);
static void nfhscale_draw_bg(NFHSCALE *hscale);
static void nfhscale_draw_bar(NFHSCALE *hscale);
static gboolean nfhscale_check_bar_innout(NFHSCALE *hscale, gint x, gint y);
static void nfhscale_draw_outlines(NFOBJECT *obj);


static gboolean nfhscale_event_handler(NFHSCALE *hscale, GdkEvent *event, gpointer data)
{

	switch(event->type) {	
		case GDK_EXPOSE:
			if(hscale->object.kfocus != NFOBJECT_FOCUS)
				hscale->object.grab_kfocus = FALSE;

			nfhscale_draw_bg(hscale);

			if(!hscale->bar_rec.x && !hscale->bar_rec.y) {
				if(hscale->object.width < 10)
					hscale->div_unit = 1;
				else 
					hscale->div_unit = (hscale->object.width - (START_X_GAP * 2)) / 10;
			}

			nfhscale_draw_bar(hscale);
			break;

		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)hscale))
				nfui_signal_emit((NFOBJECT*)hscale, GDK_EXPOSE, TRUE);
			break;

		case GDK_BUTTON_PRESS:
			{
				GdkEventButton *bevent;
				gint x, y;

				bevent = (GdkEventButton*)event;
				x = (gint)bevent->x;
				y = (gint)bevent->y;

				if(hscale->object.x + START_X_GAP > x - hscale->bar_rec.width/2) 
					break;
				else if(hscale->object.x + hscale->object.width - START_X_GAP < x + hscale->bar_rec.width/2)
					break;

				if(!nfhscale_check_bar_innout(hscale, x, y)) { 
					if(hscale->bar_rec.x != x) 
						hscale->bar_rec.x = x - (hscale->bar_rec.width/2);

					hscale->step_val = (x - hscale->object.x + START_X_GAP) / hscale->div_unit;

					nfhscale_draw_bg(hscale);
					nfhscale_draw_bar(hscale);
				}
			}
			break;
		case GDK_MOTION_NOTIFY:
			{
				GdkEventMotion *mevent;
				gint x;

				mevent = (GdkEventMotion*)event;
				x = (gint)mevent->x;

				if(hscale->object.x + START_X_GAP > x - hscale->bar_rec.width/2) 
					break;
				else if(hscale->object.x + hscale->object.width - START_X_GAP < x + hscale->bar_rec.width/2)
					break;

				if(mevent->state & (1 << 8)) {
					if(hscale->bar_rec.x != x) 
						hscale->bar_rec.x = x - (hscale->bar_rec.width/2);

					hscale->step_val = (x - hscale->object.x + START_X_GAP) / hscale->div_unit;

					nfhscale_draw_bg(hscale);
					nfhscale_draw_bar(hscale);
				}
			}
			break;
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;
				gint pos_x;

				kevt = (GdkEventKey*)event;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_ENTER) {
					if(!hscale->object.grab_kfocus) {
						hscale->object.grab_kfocus = TRUE;
						nfui_signal_emit((NFOBJECT*)hscale, GDK_EXPOSE, FALSE);

						return TRUE;
					}
				}else if(kpid == KEYPAD_EXIT) {
					if(hscale->object.grab_kfocus) {
						hscale->object.grab_kfocus = FALSE;
						nfui_signal_emit((NFOBJECT*)hscale, GDK_EXPOSE, FALSE);

						return TRUE;
					}
				}else if(kpid == KEYPAD_UP || kpid == KEYPAD_RIGHT) {
					if(!hscale->object.grab_kfocus) 
						return FALSE;

					if(hscale->step_val+1 > HSCALE_MAX_VAL)
						return TRUE;
					else {
						pos_x = (hscale->div_unit * hscale->step_val);
						pos_x += (hscale->object.x + START_X_GAP);

						if(hscale->bar_rec.x != pos_x) 
							hscale->bar_rec.x = pos_x;

						++hscale->step_val;
					}

					hscale->bar_rec.x += hscale->div_unit;

					nfhscale_draw_bg(hscale);
					nfhscale_draw_bar(hscale);

					return TRUE;

				}else if(kpid == KEYPAD_DOWN || kpid == KEYPAD_LEFT) {
					if(!hscale->object.grab_kfocus) 
						return FALSE;

					if((gint)(hscale->step_val-1) < HSCALE_MIN_VAL) 
						return TRUE;
					else {
						pos_x = (hscale->div_unit * hscale->step_val);
						pos_x += (hscale->object.x + START_X_GAP);

						if(hscale->bar_rec.x != pos_x) 
							hscale->bar_rec.x = pos_x;

						--hscale->step_val;
					}
					
					hscale->bar_rec.x -= hscale->div_unit;

					nfhscale_draw_bg(hscale);
					nfhscale_draw_bar(hscale);

					return TRUE;
					
				}
			}
			break;
		case GDK_DELETE:
			break;
		default:
			break;
	}

	return TRUE;
}



static void nfhscale_draw_bg(NFHSCALE *hscale)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkPixbuf *pm;
	guint x, y;
	
	drawable = nfui_nfobject_get_window((NFOBJECT*)hscale);
	gc = nfui_nfobject_get_gc((NFOBJECT*)hscale);

	nfui_nfobject_get_offset((NFOBJECT*)hscale, &x, &y);

	pm = hscale->bg_img;
	nfutil_draw_pixbuf(drawable, gc, pm, (gint)x, (gint)y, -1, -1, NFALIGN_LEFT, 0);

	if(hscale->object.kfocus == NFOBJECT_FOCUS) 
		nfhscale_draw_outlines((NFOBJECT*)hscale);
}

static void nfhscale_draw_bar(NFHSCALE *hscale)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkPixbuf *pm;

	drawable = nfui_nfobject_get_window((NFOBJECT*)hscale);
	gc = nfui_nfobject_get_gc((NFOBJECT*)hscale);

	pm = hscale->bar_img;
     
	if(!hscale->bar_rec.x && !hscale->bar_rec.y) {
		hscale->bar_rec.x = hscale->object.x + START_X_GAP;
		hscale->bar_rec.y = hscale->object.y;

		if(hscale->step_val)
			hscale->bar_rec.x += (hscale->div_unit * hscale->step_val);
	}

	nfutil_draw_pixbuf(drawable, gc, pm, (gint)hscale->bar_rec.x, (gint)hscale->bar_rec.y, (gint)hscale->bar_rec.width, (gint)hscale->bar_rec.height, NFALIGN_LEFT, 0);
}

static void nfhscale_draw_outlines(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);

	if(((NFHSCALE*)obj)->object.grab_kfocus)
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(NOT_CARE));
	else
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(NOT_CARE));

	gdk_gc_set_line_attributes(line_gc,
			HSCALE_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = HSCALE_LINE_BORDER - 1;
/*
	points[0].x = pos_x + line_gap;
	points[0].y = pos_y + line_gap;
	points[1].x = pos_x + obj->width - line_gap;
	points[1].y = pos_y + line_gap;
	points[2].x = pos_x + obj->width - line_gap;
	points[2].y = pos_y + obj->height - line_gap;
	points[3].x = pos_x + line_gap;
	points[3].y = pos_y + obj->height - line_gap;
	points[4].x = pos_x + line_gap;
	points[4].y = pos_y + line_gap;

	gdk_draw_lines(drawable,
			line_gc,
			points,
			5);
*/
	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);
}

static gboolean nfhscale_check_bar_innout(NFHSCALE *hscale, gint x, gint y)
{
	gint bx, by, bw, bh;
	
	bx = hscale->bar_rec.x;
	by = hscale->bar_rec.y;
	bw = hscale->bar_rec.width;
	bh = hscale->bar_rec.height;

	if((bx < x && (bx + bw)	> x)
			&& (by < y && (by + bh) > y))
		return TRUE;
	
	return FALSE;	
}



NFOBJECT* nfui_hscale_new()
{
	NFHSCALE *hscale;

	hscale = (NFHSCALE*)imalloc(sizeof(NFHSCALE));
	if(hscale == NULL)	{
		g_warning("hscale alloc is NULL");
		return NULL;
	}

#if 0
	hscale->object.parent = NULL;
	hscale->object.x = 0;
	hscale->object.y = 0;
	hscale->object.width = 160;
	hscale->object.height = 12;
	hscale->object.type = NFOBJECT_TYPE_NFHSCALE;
	hscale->object.show = NFOBJECT_HIDE;
	hscale->object.use_focus = NFOBJECT_FOCUS_ON;
	hscale->object.kfocus = NFOBJECT_UNFOCUS;
	hscale->object.mfocus = NFOBJECT_UNFOCUS;
	hscale->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	hscale->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	hscale->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	hscale->object.pre_event_handler = NULL;
	hscale->object.default_event_handler = nfhscale_event_handler;
	hscale->object.post_event_handler = NULL;
	hscale->object.grab_kfocus = FALSE;
#else
	nfui_nfobject_init((NFOBJECT*)hscale);

	hscale->object.width = 160;
	hscale->object.height = 12;
	hscale->object.type = NFOBJECT_TYPE_NFHSCALE;
	hscale->object.use_focus = NFOBJECT_FOCUS_ON;
	hscale->object.default_event_handler = nfhscale_event_handler;
#endif

	hscale->bg_img = NULL;
	hscale->bar_img = NULL;
	hscale->div_unit = 0;
	hscale->step_val = 0;

	return (NFOBJECT*)hscale;
}


void nfui_hscale_set_bg_image(NFHSCALE *hscale, GdkPixbuf *image)
{
	g_return_if_fail(hscale);
	g_return_if_fail(image);
	
	if(hscale->object.type != NFOBJECT_TYPE_NFHSCALE)
	{
		return;
	}

	hscale->bg_img = image;
}


void nfui_hscale_set_bar_image(NFHSCALE *hscale, GdkPixbuf *image, guint width, guint height)
{
	g_return_if_fail(hscale);
	g_return_if_fail(image);

	if(hscale->object.type != NFOBJECT_TYPE_NFHSCALE)
	{
		return;
	}

	hscale->bar_img = image;
	hscale->bar_rec.width = width;
	hscale->bar_rec.height = height;
}


void nfui_hscale_set_value(NFHSCALE *hscale, guint step_val)
{
	g_return_if_fail(hscale);
	g_return_if_fail(step_val >= HSCALE_MIN_VAL && step_val <= HSCALE_MAX_VAL);

	if(hscale->object.type != NFOBJECT_TYPE_NFHSCALE)
	{
		return;
	}

	if(!hscale->bar_rec.x && !hscale->bar_rec.y) {
		hscale->step_val = step_val;
	}else {
		hscale->step_val = step_val;
		hscale->bar_rec.x = (hscale->div_unit * step_val) + hscale->object.x - (hscale->bar_rec.width / 2);

		nfhscale_draw_bg(hscale);
		nfhscale_draw_bar(hscale);
	}
}


guint nfui_hscale_get_value(NFHSCALE *hscale)
{
	g_return_val_if_fail(hscale, 0);

	if(hscale->object.type != NFOBJECT_TYPE_NFHSCALE)
	{
		return 0;
	}

	return hscale->step_val;
}


