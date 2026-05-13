


#include "../../support/util.h"
#include "../../support/color.h"
#include "../../support/event_loop.h"
#include "../../support/nf_ui_color.h"

#include "nflabel.h"
#include "nfloglabel.h"
#include "ix_mem.h"


#define LOGLABEL_LINE_BORDER				2



static void nfloglabel_set_child_position(NFLOGLABEL *nfloglabel)
{
	guint i;
	guint fixed_w, fixed_h;
	guint label_x, label_w;
	
	g_return_if_fail(nfloglabel != NULL);

	label_x = 2;

	fixed_w = nfloglabel->log_fixed.object.width;	
	fixed_h = nfloglabel->log_fixed.object.height;	

	for (i = 0; i < LOGLABEL_ALL; i++)
	{
		if (i == LOGLABEL_TIME)
			label_w = nfloglabel->label_width[i];
		else
			label_w = nfloglabel->label_width[i]-2;
			
		nfui_nfobject_set_size(nfloglabel->label[i], label_w, fixed_h-4);

		nfui_nffixed_put((NFFIXED*)nfloglabel, nfloglabel->label[i], label_x, 2);
		
		label_x += (label_w + 2);
	}
}

static void nfloglabel_change_child_position(NFLOGLABEL *nfloglabel)
{
	guint i;
	guint fixed_w, fixed_h;
	guint label_x, label_w;
	
	g_return_if_fail(nfloglabel != NULL);

	label_x = 2;

	fixed_w = nfloglabel->log_fixed.object.width;	
	fixed_h = nfloglabel->log_fixed.object.height;	

	for (i = 0; i < LOGLABEL_ALL; i++)
	{
		if (i == LOGLABEL_TIME)
			label_w = nfloglabel->label_width[i];
		else
			label_w = nfloglabel->label_width[i]-2;
			
		nfui_nfobject_set_size(nfloglabel->label[i], label_w, fixed_h-4);

		((NFOBJECT *)nfloglabel->label[i])->x = label_x;
		((NFOBJECT *)nfloglabel->label[i])->y = 2;
		
		label_x += (label_w + 2);
	}
}

static void nfloglabel_draw_outlines(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;
	gint line_width;

	drawable = nfui_nfobject_get_window(obj);
	line_gc = nfui_nfobject_get_gc(obj);

	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(146));

	gdk_gc_set_line_attributes(line_gc,
			LOGLABEL_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);

	line_gap = LOGLABEL_LINE_BORDER - 1;
	line_width = obj->width;

	points[0].x = pos_x + line_gap;
	points[0].y = pos_y + line_gap;
	points[1].x = pos_x + line_width - line_gap;
	points[1].y = pos_y + line_gap;
	points[2].x = pos_x + line_width - line_gap;
	points[2].y = pos_y + obj->height - line_gap;
	points[3].x = pos_x + line_gap;
	points[3].y = pos_y + obj->height - line_gap;
	points[4].x = pos_x + line_gap;
	points[4].y = pos_y + line_gap;

	gdk_draw_lines(drawable,
			line_gc,
			points,
			5);
			
	nfui_nfobject_gc_unref(line_gc);

}

static void nfloglabel_set_gc(NFLOGLABEL *nfloglabel)
{
	GdkDrawable *drawable = NULL;

	g_return_if_fail(nfloglabel != NULL);

	drawable = nfui_nfobject_get_window((NFOBJECT*)nfloglabel);

	if(drawable) {
		nfloglabel->bg_gc = gdk_gc_new(drawable);
		gdk_gc_set_rgb_fg_color(nfloglabel->bg_gc, &UX_COLOR(nfloglabel->label_bg_idx));
	}else
		g_warning("nfloglabel drawable is NULL.");

}

static void nfloglabel_draw_bg(NFLOGLABEL *nfloglabel)
{
	GdkDrawable *drawable = NULL;
	guint px, py;

	g_return_if_fail(nfloglabel != NULL);

	if(nfui_nfobject_is_disabled((NFOBJECT*)nfloglabel))
		return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)nfloglabel);
	nfui_nfobject_get_offset((NFOBJECT*)nfloglabel, &px, &py);

	if(drawable)
	{
		gdk_draw_rectangle(drawable,
				nfloglabel->bg_gc,
				TRUE,
				(gint)px,
				(gint)py,
				(gint)(nfloglabel->label_width[LOGLABEL_TYPE]),
				(gint)(nfloglabel->log_fixed.object.height));

		px += (nfloglabel->label_width[LOGLABEL_TYPE] + 2);

		gdk_draw_rectangle(drawable,
				nfloglabel->bg_gc,
				TRUE,
				(gint)px,
				(gint)py,
				(gint)(nfloglabel->label_width[LOGLABEL_TIME]),
				(gint)(nfloglabel->log_fixed.object.height));

		px += (nfloglabel->label_width[LOGLABEL_TIME] + 2);

		gdk_draw_rectangle(drawable,
				nfloglabel->bg_gc,
				TRUE,
				(gint)px,
				(gint)py,
				(gint)(nfloglabel->label_width[LOGLABEL_SETBY]),
				(gint)(nfloglabel->log_fixed.object.height));		
	}
	else
		g_warning("nfloglabel drawable is NULL.");

	if(nfloglabel->log_fixed.object.kfocus == NFOBJECT_FOCUS) 
		nfloglabel_draw_outlines((NFOBJECT*)nfloglabel);
}

static gboolean nfloglabel_event_handler(NFLOGLABEL *nfloglabel, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;

	switch(event->type)
	{
		case GDK_EXPOSE :
		{
			guint i;
		
			if(!nfloglabel->bg_gc) 
				nfloglabel_set_gc(nfloglabel);

			if(nfloglabel->bg_gc) 
				nfloglabel_draw_bg(nfloglabel);

			for (i = 0; i < LOGLABEL_ALL; i++)
			{
				if(nfloglabel->strLabel[i])
				{
					nfui_nflabel_set_text((NFLABEL*)(nfloglabel->label[i]), nfloglabel->strLabel[i]);
					nfui_signal_emit((NFLABEL*)(nfloglabel->label[i]), GDK_EXPOSE, TRUE);
				}					
			}

			// SKSHIN
			//if(!((NFFIXED*)nfloglabel)->children)
			//	nfloglabel_set_child_position(nfloglabel);
			
			nfloglabel_change_child_position(nfloglabel);

		}
		break;

		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
//			if(nfui_nfobject_is_shown((NFOBJECT*)nfloglabel))
//				nfui_signal_emit((NFOBJECT*)nfloglabel, GDK_EXPOSE, TRUE);
			break;

		case GDK_2BUTTON_PRESS :
			break;

		case GDK_BUTTON_PRESS :
			break;

		case GDK_BUTTON_RELEASE :
			break;

		case GDK_DELETE :
			if(nfloglabel->bg_gc)
				g_object_unref(nfloglabel->bg_gc);
		
			break;

		default :
			break;
	}

	return FALSE;
}

static gboolean nfloglabel_label_cb(NFOBJECT *label, GdkEvent *event, gpointer data)
{
	NFLOGLABEL *loglabel;
	NFOBJECT *item;

	loglabel = NF_LOGLABEL(label->parent);

	switch(event->type) {
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON) 					
					return FALSE;

				if(nfui_is_focus_at_child((NFOBJECT*)loglabel)) {
					nfui_set_key_focus((NFOBJECT*)loglabel, TRUE);					
					nfui_signal_emit((NFOBJECT*)loglabel, GDK_EXPOSE, TRUE);
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

NFLOGLABEL* nfui_nfloglabel_new(gchar *pfont, guint fg_idx, guint bg_idx)
{
	NFLOGLABEL *nfloglabel;
	guint i;

	nfloglabel = (NFLOGLABEL*)imalloc(sizeof(NFLOGLABEL));
	if(nfloglabel == NULL)	return NULL;

	nfui_nfobject_init((NFOBJECT*)nfloglabel);

	nfloglabel->log_fixed.object.width = 140;
	nfloglabel->log_fixed.object.height = 22;
	nfloglabel->log_fixed.object.type = NFOBJECT_TYPE_NFLOGLABEL;
	nfloglabel->log_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	nfloglabel->log_fixed.object.default_event_handler = nfloglabel_event_handler;
	nfloglabel->spacing_type = NORMAL_SPACING;

	for (i = 0; i < LOGLABEL_ALL; i++)
	{
		nfloglabel->label[i] = (NFOBJECT*)nfui_nflabel_new("");
		nfui_nflabel_set_drawing_outline((NFLABEL*)(nfloglabel->label[i]), FALSE);

		if (i == LOGLABEL_TIME)
			nfui_nflabel_set_align((NFLABEL*)nfloglabel->label[i], NFALIGN_CENTER, 0);
		else
			nfui_nflabel_set_align((NFLABEL*)nfloglabel->label[i], NFALIGN_LEFT, 10);
		
		nfui_nflabel_set_pango_font((NFLABEL*)nfloglabel->label[i], pfont, fg_idx);		
		nfui_regi_post_event_callback(nfloglabel->label[i], (gpointer)nfloglabel_label_cb);
		nfui_nfobject_modify_bg(nfloglabel->label[i], NFOBJECT_STATE_NORMAL, bg_idx);
		nfui_nfobject_show(nfloglabel->label[i]); 	
	}

	// SKSHIN
	nfloglabel_set_child_position(nfloglabel);

	nfloglabel->align = NFALIGN_CENTER;
	nfloglabel->margin = 0;
	nfloglabel->spacing_type = NORMAL_SPACING;

	memset(&nfloglabel->log_time, 0, sizeof(GTimeVal));

	return nfloglabel;
}

void nfui_nfloglabel_label_size_width(NFLOGLABEL *nfloglabel, LOGLABEL_E label_idx, guint size_w)
{
	guint i;

	g_return_if_fail(nfloglabel != NULL); 
	
	if(nfloglabel->log_fixed.object.type != NFOBJECT_TYPE_NFLOGLABEL)
	{
		return;
	}

	if (label_idx == LOGLABEL_ALL)
	{
		for (i = 0; i < LOGLABEL_ALL; i++)
			nfloglabel->label_width[i] = size_w;
	}
	else
	{
		nfloglabel->label_width[label_idx] = size_w;
	}
}

void nfui_nfloglabel_label_bg(NFLOGLABEL *nfloglabel, guint color_idx)
{
	guint i;

	g_return_if_fail(nfloglabel != NULL); 
	
	if(nfloglabel->log_fixed.object.type != NFOBJECT_TYPE_NFLOGLABEL)
	{
		return;
	}

	nfloglabel->label_bg_idx = color_idx;
}

void nfui_nfloglabel_label_set_text(NFLOGLABEL *nfloglabel, LOGLABEL_E label_idx, gchar *strLabel)
{
	guint i;
	gint length;

	if(nfloglabel->log_fixed.object.type != NFOBJECT_TYPE_NFLOGLABEL)
	{
		return;
	}

	memset(nfloglabel->strLabel[label_idx], 0, sizeof(nfloglabel->strLabel[label_idx]));
	length = strlen(strLabel);

	if(length > 0)
	{
		strncpy(nfloglabel->strLabel[label_idx], strLabel, sizeof(gchar)*length);
//		nfui_signal_emit((NFOBJECT*)nfloglabel, GDK_EXPOSE, TRUE);
	}

//	nfui_nflabel_set_text((NFLABEL*)nfloglabel->label[label_idx], strLabel);
//	nfui_signal_emit((NFLABEL*)nfloglabel->label[label_idx], GDK_EXPOSE, TRUE);
//	nfui_signal_emit((NFOBJECT*)nfloglabel, GDK_EXPOSE, TRUE);
}

void nfui_nfloglabel_set_log_time(NFLOGLABEL *nfloglabel, GTimeVal log_time)
{
	nfloglabel->log_time = log_time;
}

void nfui_nfloglabel_get_log_time(NFLOGLABEL *nfloglabel, GTimeVal *log_time)
{
	*log_time = nfloglabel->log_time;
}

