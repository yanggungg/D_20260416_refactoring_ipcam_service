#include <string.h>

#include "../../support/util.h"
#include "../../support/event_loop.h"
#include "../../support/color.h"


#include "nftimeline.h"
#include "nf_codec_header.h"
#include "ix_mem.h"


#define	NFTL_BORDER_SIZE	(2)


static gboolean nftimeline_event_handler(NFTIMELINE *timeline, GdkEvent *event, gpointer data);
static void nftimeline_set_bg_gc(NFTIMELINE *timeline);
static void nftimeline_draw_tbar(NFTIMELINE *timeline);
static void nftimeline_clear_tbar(NFTIMELINE *timeline);
static void nftimeline_draw_sbar(NFTIMELINE *timeline);
static void nftimeline_draw_bg(NFTIMELINE *timeline);
static void nftimeline_draw_outlines(NFOBJECT *obj);
static void nftimeline_change_pos(NFTIMELINE *timeline, gint new_pos);


static void nftimeline_set_bg_gc(NFTIMELINE *timeline)
{
	GdkDrawable *drawable = NULL;
	GdkColor bg_color = UX_COLOR(NOT_CARE);
	GdkColor tbar_color = UX_COLOR(NOT_CARE);
	GdkColor sbar_color[EVENT_TYPE_COUNT] = {UX_COLOR(NOT_CARE),
											UX_COLOR(NOT_CARE),
											UX_COLOR(NOT_CARE),
											UX_COLOR(NOT_CARE),
											UX_COLOR(NOT_CARE),
											UX_COLOR(NOT_CARE)};
	gint i;

	drawable = nfui_nfobject_get_window((NFOBJECT*)timeline);

	timeline->bg_gc = gdk_gc_new(drawable);
	gdk_gc_set_rgb_fg_color(timeline->bg_gc, &bg_color);

	timeline->odd_gc = gdk_gc_new(drawable);
	gdk_gc_set_rgb_fg_color(timeline->odd_gc, &UX_COLOR(timeline->row_color[0]));

	timeline->even_gc = gdk_gc_new(drawable);
	gdk_gc_set_rgb_fg_color(timeline->even_gc, &UX_COLOR(timeline->row_color[1]));

	timeline->tbar_gc = gdk_gc_new(drawable);
	gdk_gc_set_rgb_fg_color(timeline->tbar_gc, &tbar_color);

#if defined(__SAMSUNG_UI__)
	timeline->inline_gc = gdk_gc_new(drawable);
	gdk_gc_set_rgb_fg_color(timeline->inline_gc, &UX_COLOR(NOT_CARE));
#endif
	
	for(i=0; i<EVENT_TYPE_COUNT; i++) {
		timeline->sbar_gc[i] = gdk_gc_new(drawable);
		gdk_gc_set_rgb_fg_color(timeline->sbar_gc[i], &sbar_color[i]);
	}
}

static void nftimeline_change_pos(NFTIMELINE *timeline, gint new_pos)
{
	nftimeline_clear_tbar(timeline);
	timeline->cur_pos = new_pos;
	nftimeline_draw_tbar(timeline);	
}


static void nftimeline_draw_bg(NFTIMELINE *timeline)
{
	GdkDrawable *drawable = NULL;
	guint off_x, off_y;
	gint x, y, w, h;
	gint i;

	drawable = nfui_nfobject_get_window((NFOBJECT*)timeline);
	nfui_nfobject_get_offset((NFOBJECT*)timeline, &off_x, &off_y);

	if(timeline->object.kfocus == NFOBJECT_FOCUS)
		nftimeline_draw_outlines((NFOBJECT*)timeline);

	x = off_x + timeline->da_x;
	y = off_y + timeline->da_y;
	w = timeline->da_w;
	h = timeline->cell_height;

	for(i=0; i<timeline->rows; i++)
	{
		if(i%2)	gdk_draw_rectangle(drawable, timeline->odd_gc, TRUE, x, y, w, h);
		else	gdk_draw_rectangle(drawable, timeline->even_gc, TRUE, x, y, w, h);
		y += (timeline->cell_height + timeline->row_space);
	}
}

#if defined(__SAMSUNG_UI__)
static void nftimeline_draw_inlines(NFTIMELINE *timeline, GdkDrawable *drawable, gint off_x, gint off_y)
{
	gint x, y, w, h;
	gint i;

	x = off_x + timeline->da_x;
	y = off_y + timeline->da_y;
	w = timeline->da_w;
	h = timeline->cell_height;

	for(i=0; i<timeline->rows; i++) {
		gdk_draw_rectangle(drawable, timeline->inline_gc, FALSE, x, y, w - 1, h - 1);
		y += (timeline->cell_height + timeline->row_space);
	}
}
#endif

static void nftimeline_draw_tbar(NFTIMELINE *timeline)
{
	GdkDrawable *drawable = NULL;
	gint off_x, off_y;
	gint x, y, w, h;
	gint i;

	if(!timeline || !timeline->bg_gc)
		return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)timeline);

	if(!drawable)
		return;

	nfui_nfobject_get_offset((NFOBJECT*)timeline, &off_x, &off_y);

#if defined(__SAMSUNG_UI__)
	nftimeline_draw_inlines(timeline, drawable, off_x, off_y);
#endif


	x = off_x + timeline->da_x + (timeline->cur_pos*timeline->cell_width) - 3;
	y = off_y + timeline->da_y;
	w = 8;
	h = 2;

	if(x < off_x + timeline->da_x)
	{
		w -= (off_x+timeline->da_x - x);
		x = off_x + timeline->da_x;
	}
	else if(x + w > off_x + timeline->da_x + timeline->da_w)
	{
		w -= x + w - (off_x+timeline->da_x + timeline->da_w); 
	}

	gdk_draw_rectangle(drawable, timeline->tbar_gc, TRUE, x, y, w, h);

	x = off_x + timeline->da_x + (timeline->cur_pos*timeline->cell_width) - 2;
	y += 2;
	w = 6;
	if(x < off_x + timeline->da_x)
	{
		w -= (off_x+timeline->da_x - x);
		x = off_x + timeline->da_x;
	}
	else if(x + w > off_x + timeline->da_x + timeline->da_w)
	{
		w -= x + w- (off_x+timeline->da_x + timeline->da_w); 
	}

	gdk_draw_rectangle(drawable, timeline->tbar_gc, TRUE, x, y, w, h);

	x = off_x + timeline->da_x + (timeline->cur_pos*timeline->cell_width) - 1;
	y += 2;
	w = 4;
	if(x < off_x + timeline->da_x)
	{
		w -= (off_x+timeline->da_x - x);
		x = off_x + timeline->da_x;
	}
	else if(x + w > off_x + timeline->da_x + timeline->da_w)
	{
		w -= x + w - (off_x+timeline->da_x + timeline->da_w); 
	}
	gdk_draw_rectangle(drawable, timeline->tbar_gc, TRUE, x, y, w, h);


	x = off_x + timeline->da_x + (timeline->cur_pos*timeline->cell_width);
	y = off_y + timeline->da_y;
	w = timeline->cell_width;
	h = timeline->da_h;

	gdk_draw_rectangle(drawable, timeline->tbar_gc, TRUE, x, y, w, h);
}

static void nftimeline_clear_tbar(NFTIMELINE *timeline)
{
	NFOBJECT *obj_temp = NULL;
	GdkDrawable *drawable = NULL;
	NF_RECORD_REASON_E reason = NF_RECORD_REASON_NOTHING;
	GdkColor bg_color = UX_COLOR(NOT_CARE);

	gint bg_color_idx;
	gint off_x, off_y;
	gint x, y, w, h;
	gint i;
	gint start_pos, step;

	nfui_nfobject_get_offset((NFOBJECT*)timeline, &off_x, &off_y);
	drawable = nfui_nfobject_get_window((NFOBJECT*)timeline);

	if(!drawable)	return;

	/* MAIN BG */
	obj_temp = (NFOBJECT*)timeline;

	while(obj_temp->bg_color[NFOBJECT_STATE_NORMAL] == -1)
	{
		if(obj_temp->type == NFOBJECT_TYPE_TOP)
			break;

		if(obj_temp->parent == NULL)
			break;
			
		obj_temp = obj_temp->parent;
	}

	bg_color_idx = obj_temp->bg_color[NFOBJECT_STATE_NORMAL];

	x = off_x + timeline->da_x + (timeline->cur_pos*timeline->cell_width);
	y = off_y + timeline->da_y;

	gdk_gc_set_rgb_fg_color(timeline->bg_gc, &UX_COLOR(bg_color_idx));
	gdk_draw_rectangle(drawable, timeline->bg_gc, TRUE, x, y, timeline->cell_width, timeline->da_h);


	/* DRAW SBAR BG OF FIRST ROW */
	x = off_x + timeline->da_x + (timeline->cur_pos*timeline->cell_width) - 3;
	y = off_y + timeline->da_y;
	w = 8;
	h = timeline->cell_height;

	if(x < off_x + timeline->da_x)
	{
		w -= (off_x+timeline->da_x - x);
		x = off_x + timeline->da_x;
	}
	else if(x + w > off_x + timeline->da_x + timeline->da_w)
	{
		w -= x + w - (off_x+timeline->da_x + timeline->da_w); 
	}
	gdk_draw_rectangle(drawable, timeline->even_gc, TRUE, x, y, w, h);


	/* DRAW SBAR OF FIRST ROW */
	if(timeline->cur_pos >= 0 && timeline->cur_pos < 2)
	{
		start_pos = 0;
		step = timeline->cur_pos + 3;
	}
	else if(timeline->cur_pos >= 2 && timeline->cur_pos < timeline->columns-2)
	{
		start_pos = timeline->cur_pos - 2;
		step = 5;
	}
	else if(timeline->cur_pos >= timeline->columns-2 && timeline->cur_pos < timeline->columns)
	{
		start_pos = timeline->cur_pos - 2;
		step = 3 + ((timeline->columns - 1) - timeline->cur_pos);
	}
	else
	{
		start_pos = 0;
		step = 0;
	}

	x = off_x + timeline->da_x + timeline->cell_width*start_pos;
	y = off_y + timeline->da_y + ((timeline->cell_height-timeline->data_height)/2);
	w = timeline->cell_width;
	h = timeline->data_height;

	if(timeline->t_data)
	{
		for(i=0; i<step; i++)
		{
			reason = timeline->t_data[start_pos+i];
			if(reason > NF_RECORD_REASON_NOTHING && reason <= NF_RECORD_REASON_PRE)
				gdk_draw_rectangle(drawable, timeline->sbar_gc[reason - 1], TRUE, x, y, timeline->cell_width, timeline->data_height);

			x += timeline->cell_width;
		}
	}


	/* DRAW SBAR BG FROM SECOND ROW TO LAST ROW */
	x = off_x + timeline->da_x + timeline->cell_width*timeline->cur_pos;
	y = off_y + timeline->da_y;
	w = timeline->cell_width;
	h = timeline->cell_height;

	for(i=1; i<timeline->rows; i++)
	{
		y += (timeline->cell_height + timeline->row_space);

		if(i%2)	gdk_draw_rectangle(drawable, timeline->odd_gc, TRUE, x, y, w, h);
		else	gdk_draw_rectangle(drawable, timeline->even_gc, TRUE, x, y, w, h);
	}

	/* DRAW SBAR FROM SECOND ROW TO LAST ROW */
	x = off_x + timeline->da_x + timeline->cell_width*timeline->cur_pos;
	y = off_y + timeline->da_y + ((timeline->cell_height-timeline->data_height)/2);
	w = timeline->cell_width;
	h = timeline->data_height;

	if(timeline->t_data)
	{
		for(i=1; i<timeline->rows; i++)
		{
			y += (timeline->cell_height + timeline->row_space);

			reason = timeline->t_data[timeline->columns*i+timeline->cur_pos];
			if(reason > NF_RECORD_REASON_NOTHING && reason <= NF_RECORD_REASON_PRE)
				gdk_draw_rectangle(drawable, timeline->sbar_gc[reason - 1], TRUE, x, y, w, h);
		}
	}
}

static void nftimeline_draw_sbar(NFTIMELINE *timeline)
{
	GdkDrawable *drawable = NULL;
	NF_RECORD_REASON_E reason_temp = NF_RECORD_REASON_NOTHING;
	NF_RECORD_REASON_E reason = NF_RECORD_REASON_NOTHING;
	guint i = 0, j = 0;
	guint columns;
	guint off_x, off_y;
	gint x, y, w, h;
	gint tx, ty, tw;
	gint reason_count;

	drawable = nfui_nfobject_get_window((NFOBJECT*)timeline);
	nfui_nfobject_get_offset((NFOBJECT*)timeline, &off_x, &off_y);

	columns = timeline->columns;

	x = off_x + timeline->da_x;
	y = off_y + timeline->da_y + ((timeline->cell_height-timeline->data_height)/2);
	h = timeline->data_height;

	ty = y;

	
	for(i=0; i<timeline->rows; i++)
	{
		reason_count = 0;
		reason_temp = timeline->t_data[columns*i];

		for(j=0; j<columns; j++)
		{
			reason = timeline->t_data[columns*i+j];

			if(reason == reason_temp)
			{
				reason_count++;
				if(j < columns-1)	continue;
			}

			if(reason_temp > NF_RECORD_REASON_NOTHING && reason_temp <= NF_RECORD_REASON_PRE)
			{
				tx = x + (timeline->cell_width * (j-reason_count));
				tw = timeline->cell_width * reason_count;
				gdk_draw_rectangle(drawable, timeline->sbar_gc[reason_temp - 1], TRUE, tx, ty, tw, h);
			}

			if(j == timeline->columns-1 &&
					reason > NF_RECORD_REASON_NOTHING && reason <= NF_RECORD_REASON_PRE)
			{
				tx = x + (timeline->cell_width * j);
				tw = timeline->cell_width;
				gdk_draw_rectangle(drawable, timeline->sbar_gc[reason - 1], TRUE, tx, ty, tw, h);
			}

			reason_temp = reason;
			reason_count = 1;
		}
		ty += (timeline->cell_height + timeline->row_space);
	}
}


static void nftimeline_draw_outlines(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint off_x, off_y;
	gint x, y, w, h;
	NFTIMELINE *timeline = NULL;

	drawable = nfui_nfobject_get_window(obj);
	timeline = (NFTIMELINE*)obj;

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);

	if(obj->grab_kfocus)
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(NOT_CARE));
	else
#if defined(__CTYPE_UI__)
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(NOT_CARE));
#else
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(NOT_CARE));
#endif

	nfui_nfobject_get_offset(obj, &off_x, &off_y);

	x = off_x;
	y = off_y;
	w = timeline->object.width;
	h = NFTL_BORDER_SIZE;
	gdk_draw_rectangle(drawable, line_gc, TRUE, x, y, w, h);

	x = off_x;
	y = off_y + timeline->da_y;;
	w = NFTL_BORDER_SIZE;
	h = timeline->da_h;
	gdk_draw_rectangle(drawable, line_gc, TRUE, x, y, w, h);

	x = off_x;
	y = off_y + timeline->da_y + timeline->da_h;
	w = timeline->object.width;
	h = NFTL_BORDER_SIZE;
	gdk_draw_rectangle(drawable, line_gc, TRUE, x, y, w, h);

	x = off_x + timeline->da_x + timeline->da_w;
	y = off_y + timeline->da_y;
	w = NFTL_BORDER_SIZE;
	h = timeline->da_h;
	gdk_draw_rectangle(drawable, line_gc, TRUE, x, y, w, h);

	nfui_nfobject_gc_unref(line_gc);
}

static gboolean nftimeline_event_handler(NFTIMELINE *timeline, GdkEvent *event, gpointer data)
{
	switch(event->type) {	
		case GDK_EXPOSE:
		{
			guint off_x, off_y, h;
			gint i;

			if(timeline->object.kfocus != NFOBJECT_FOCUS)
				timeline->object.grab_kfocus = FALSE;

			if(!timeline->bg_gc)
				nftimeline_set_bg_gc(timeline);

			nftimeline_draw_bg(timeline);

			if(timeline->t_data)
				nftimeline_draw_sbar(timeline);

			nftimeline_draw_tbar(timeline);	

		}
		break;

		case GDK_MOTION_NOTIFY:
		case GDK_BUTTON_PRESS:
		{	
			gint x, y;
			guint off_x, off_y;
			gint temp;

			x = 0;
			y = 0;

			x = ((GdkEventMotion*)event)->x;
			y = ((GdkEventMotion*)event)->y;

			if(event->type == GDK_MOTION_NOTIFY)
			{
				if(!timeline->is_pressed)
					break;
			}
			else if(event->type == GDK_BUTTON_PRESS)
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON)
					return FALSE;
				
				timeline->is_pressed = TRUE;
				timeline->object.grab_kfocus = FALSE;
				nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, FALSE);
			}

			nfui_nfobject_get_offset((NFOBJECT*)timeline, &off_x, &off_y);

			off_x += timeline->da_x;
			off_y += timeline->da_y;

			if(x < off_x || x >= off_x + timeline->da_w)
				break;
			if(y < off_y || y >= off_y + timeline->da_h)
				break;

			temp = (x - off_x) / timeline->cell_width;

			if(temp >= timeline->columns)	temp = timeline->columns-1;
			else if(temp < 0)				temp = 0;

#if 0
			timeline->cur_pos = temp;
			nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, TRUE);
#else
			nftimeline_change_pos(timeline, temp);
#endif
		}
		break;

		case GDK_BUTTON_RELEASE:
		{
			if(event->button.button == MOUSE_RIGTH_BUTTON)
				return FALSE;
			
			if(timeline->is_pressed)
				timeline->is_pressed = FALSE;

			nfui_user_signal_emit((NFOBJECT*)timeline, NFEVENT_TIMELINE_CHANGED, FALSE);
		}
		break;

		
		case GDK_LEAVE_NOTIFY:
			if(timeline->is_pressed) {
				timeline->is_pressed = FALSE;
				nfui_user_signal_emit((NFOBJECT*)timeline, NFEVENT_TIMELINE_CHANGED, FALSE);
			}
		case GDK_ENTER_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)timeline))
				nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, TRUE);
			break;



		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;
			guint off_x, off_y;
			gint gap;
			gint i;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			nfui_nfobject_get_offset((NFOBJECT*)timeline, &off_x, &off_y);

			if(kpid == KEYPAD_ENTER) {
				if(timeline->object.grab_kfocus) 
					timeline->object.grab_kfocus = FALSE;
				else
					timeline->object.grab_kfocus = TRUE;

				nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, TRUE);
				return TRUE;
			}else if(kpid == KEYPAD_EXIT) {
				if(timeline->object.grab_kfocus) {
					timeline->object.grab_kfocus = FALSE;

					nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, TRUE);
					return TRUE;
				}
				return FALSE;
			}else if(kpid == KEYPAD_RIGHT) {
				if(!timeline->object.grab_kfocus) 
					return FALSE;

				if(timeline->cur_pos < timeline->columns-1)
				{
#if 0
					(timeline->cur_pos)++;
					nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, TRUE);
					nfui_user_signal_emit((NFOBJECT*)timeline, NFEVENT_TIMELINE_CHANGED, FALSE);
#else
					nftimeline_change_pos(timeline, timeline->cur_pos+1);
					nfui_user_signal_emit((NFOBJECT*)timeline, NFEVENT_TIMELINE_CHANGED, FALSE);
#endif
				}

				return TRUE;

			}else if(kpid == KEYPAD_LEFT) {
				if(!timeline->object.grab_kfocus) 
					return FALSE;

				if(timeline->cur_pos > 0)
				{
#if 0
					(timeline->cur_pos)--;
					nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, TRUE);
					nfui_user_signal_emit((NFOBJECT*)timeline, NFEVENT_TIMELINE_CHANGED, FALSE);
#else
					nftimeline_change_pos(timeline, timeline->cur_pos-1);
					nfui_user_signal_emit((NFOBJECT*)timeline, NFEVENT_TIMELINE_CHANGED, FALSE);
#endif
				}

				return TRUE;
			}
			else
			{
				if(timeline->object.grab_kfocus) 
					return TRUE;
				else
					return FALSE;
			}
		}
		break;

		case GDK_DELETE:
		{
			gint i;

			if(timeline->bg_gc)		g_object_unref(timeline->bg_gc);
			if(timeline->odd_gc)	g_object_unref(timeline->odd_gc);
			if(timeline->even_gc)	g_object_unref(timeline->even_gc);

			timeline->bg_gc = NULL;
			timeline->odd_gc = NULL;
			timeline->even_gc = NULL;


			if(timeline->tbar_gc)	g_object_unref(timeline->tbar_gc);
			timeline->tbar_gc = NULL;

			for(i=0; i<EVENT_TYPE_COUNT; i++)
			{
				if(timeline->sbar_gc[i])	g_object_unref(timeline->sbar_gc[i]);
				timeline->sbar_gc[i] = NULL;
			}

			if(timeline->t_data) 	g_free(timeline->t_data);
			timeline->t_data = NULL;

			timeline = NULL;
		}
		break;

		default:
			break;
	}

	return FALSE;
}


NFOBJECT* nfui_timeline_new(guint cell_w, guint cell_h, guint cols, guint rows, guint row_space)
{
	NFTIMELINE *timeline;

	timeline = (NFTIMELINE*)imalloc(sizeof(NFTIMELINE));
	if(timeline == NULL)	{
		g_warning("timeline alloc is NULL");
		return NULL;
	}

#if 0
	timeline->object.parent = NULL;
	timeline->object.x = 0;
	timeline->object.y = 0;
	timeline->object.width = cell_w*cols + (NFTL_BORDER_SIZE * 2);
	timeline->object.height = (cell_h + row_space)*rows - row_space + (NFTL_BORDER_SIZE*2);
	timeline->object.type = NFOBJECT_TYPE_NFTIMELINE;
	timeline->object.show = NFOBJECT_HIDE;
	timeline->object.use_focus = NFOBJECT_FOCUS_ON;
	timeline->object.kfocus = NFOBJECT_UNFOCUS;
	timeline->object.mfocus = NFOBJECT_UNFOCUS;
	timeline->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	timeline->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	timeline->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	timeline->object.pre_event_handler = NULL;
	timeline->object.default_event_handler = nftimeline_event_handler;
	timeline->object.post_event_handler = NULL;
	timeline->object.grab_kfocus = 0;
#else
	nfui_nfobject_init((NFOBJECT*)timeline);

	timeline->object.width = cell_w*cols + (NFTL_BORDER_SIZE * 2);
	timeline->object.height = (cell_h + row_space)*rows - row_space + (NFTL_BORDER_SIZE*2);
	timeline->object.type = NFOBJECT_TYPE_NFTIMELINE;
	timeline->object.use_focus = NFOBJECT_FOCUS_ON;
	timeline->object.default_event_handler = nftimeline_event_handler;
#endif

	timeline->odd_gc = NULL;
	timeline->even_gc = NULL;
	timeline->bg_gc = NULL;
	timeline->tbar_gc = NULL;
#if defined(__SAMSUNG_UI__)
	timeline->inline_gc = NULL;	
#endif

	memset(timeline->row_color, -1, sizeof(gint)*2);
	timeline->row_color[0] = COLOR_IDX(NOT_CARE);
	timeline->row_color[1] = COLOR_IDX(NOT_CARE);

	memset(timeline->sbar_gc, 0, sizeof(timeline->sbar_gc));	

	timeline->cell_width = cell_w;
	timeline->cell_height = cell_h;
	timeline->data_height = 8;
	
	timeline->columns = cols;
	timeline->rows = rows;

	timeline->row_space = row_space;


	timeline->cur_pos = 0;
	timeline->is_pressed = 0;

	timeline->da_x = NFTL_BORDER_SIZE;
	timeline->da_y = NFTL_BORDER_SIZE;
	timeline->da_w = timeline->object.width - (NFTL_BORDER_SIZE*2);
	timeline->da_h = timeline->object.height - (NFTL_BORDER_SIZE*2);

	timeline->t_data = NULL;

	return (NFOBJECT*)timeline;
}


void nfui_timeline_set_data(NFTIMELINE *timeline, gchar *t_elem)
{

	g_return_if_fail(timeline != NULL);
	g_return_if_fail(t_elem != NULL);

	if(timeline->object.type != NFOBJECT_TYPE_NFTIMELINE)
	{
		return;
	}

	if(timeline->t_data)	g_free(timeline->t_data);
	timeline->t_data = NULL;

	timeline->t_data = g_memdup(t_elem, sizeof(gchar)*(timeline->columns * timeline->rows));
}

void nfui_timeline_set_position(NFTIMELINE *timeline, guint pos)
{
	if(timeline->object.type != NFOBJECT_TYPE_NFTIMELINE)
	{
		return;
	}

#if 0
	timeline->cur_pos = pos;

	if(timeline->bg_gc) 
		nfui_signal_emit((NFOBJECT*)timeline, GDK_EXPOSE, TRUE);
#else
	if(timeline->bg_gc) 
		nftimeline_change_pos(timeline, pos);
	else
		timeline->cur_pos = pos;
#endif
}


guint nfui_timeline_get_position(NFTIMELINE *timeline)
{
	if(timeline->object.type != NFOBJECT_TYPE_NFTIMELINE)
	{
		return 0;
	}

	return timeline->cur_pos;
}


void nfui_timeline_set_row_color(NFTIMELINE *timeline, gint odd_color_idx, gint even_color_idx)
{
	if(timeline->object.type != NFOBJECT_TYPE_NFTIMELINE)
	{
		return;
	}

	timeline->row_color[0] = odd_color_idx;
	timeline->row_color[1] = even_color_idx;

	if(timeline->odd_gc)
		gdk_gc_set_rgb_fg_color(timeline->odd_gc, &UX_COLOR(timeline->row_color[0]));
	if(timeline->even_gc)
		gdk_gc_set_rgb_fg_color(timeline->even_gc, &UX_COLOR(timeline->row_color[1]));
}


void nfui_timeline_set_data_height(NFTIMELINE *timeline, gint data_height)
{
	if(timeline->object.type != NFOBJECT_TYPE_NFTIMELINE)
	{
		return;
	}

	timeline->data_height = data_height;
}




