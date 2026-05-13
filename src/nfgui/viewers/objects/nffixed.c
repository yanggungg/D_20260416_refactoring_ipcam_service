
/**********************************************************************************
 *
 *	NFFIXED
 *	
 * *******************************************************************************/

#include "../../support/color.h"
#include "../../support/event_loop.h"
#include "nffixed.h"
#include "nfscrolledfixed.h"
#include "ix_mem.h"


#define FIXED_LINE_BORDER				2

static void prvDrawOutline(NFOBJECT *obj)
{
	NFFIXED *nffixed;
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(146));

	gdk_gc_set_line_attributes(line_gc,
			FIXED_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = FIXED_LINE_BORDER - 1;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));
	nfui_nfobject_gc_unref(line_gc);

}


static gboolean nffixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFFIXED *nffixed;

	nffixed = (NFFIXED*)obj;

	switch(event->type)
	{
		case GDK_EXPOSE:
			if(nffixed->draw_outline && nffixed->object.kfocus == NFOBJECT_FOCUS) 
				prvDrawOutline((NFOBJECT*)nffixed);
			break;

		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			if(nffixed->draw_outline && nfui_nfobject_is_shown(obj))
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			break;

		case GDK_DELETE :
			if(nffixed && nffixed->children)
				g_slist_free(nffixed->children);

			nffixed->children = NULL;

			break;

		default:
		break;
	}

	return FALSE;
}

NFFIXED *nfui_nffixed_new()
{
	NFFIXED *fixed;

	fixed = (NFFIXED*)imalloc(sizeof(NFFIXED));
	if(fixed == NULL)	return NULL;

#if 0
	fixed->object.parent = NULL;
	fixed->object.x = 0;
	fixed->object.y = 0;
	fixed->object.width = 100;
	fixed->object.height = 100;
	fixed->object.type = NFOBJECT_TYPE_NFFIXED;
	fixed->object.show = NFOBJECT_HIDE;
	fixed->object.use_focus = NFOBJECT_FOCUS_OFF;
	fixed->object.kfocus = NFOBJECT_UNFOCUS;
	fixed->object.mfocus = NFOBJECT_UNFOCUS;
	fixed->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	fixed->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	fixed->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	fixed->object.pre_event_handler = NULL;
	fixed->object.default_event_handler = nffixed_event_handler;
	fixed->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)fixed);

	fixed->object.width = 100;
	fixed->object.height = 100;
	fixed->object.type = NFOBJECT_TYPE_NFFIXED;
	fixed->object.default_event_handler = nffixed_event_handler;
#endif

	fixed->children = NULL;
	fixed->draw_outline = FALSE;

	return fixed;
}

void nfui_nffixed_destroy(NFFIXED *nffixed)
{
	if(nffixed->object.type != NFOBJECT_TYPE_NFFIXED && 
		nffixed->object.type != NFOBJECT_TYPE_NFCALENDAR && 
		nffixed->object.type != NFOBJECT_TYPE_NFCOMBOBOX && 
		nffixed->object.type != NFOBJECT_TYPE_NFLISTBOX && 
		nffixed->object.type != NFOBJECT_TYPE_NFSPINBUTTON && 
		nffixed->object.type != NFOBJECT_TYPE_NFTABLE && 
		nffixed->object.type != NFOBJECT_TYPE_NFTIMESPIN &&
		nffixed->object.type != NFOBJECT_TYPE_NFLOGLABEL && 
		nffixed->object.type != NFOBJECT_TYPE_NFTHUMBNAIL && 
		nffixed->object.type != NFOBJECT_TYPE_NFTIMELABEL ) 
	{
		return;
	}

	if(nffixed == NULL)	return;

	if(nffixed)
	{
		guint i, child_num;
		NFOBJECT *child;

		child_num = g_slist_length(nffixed->children);

		for(i=0; i<child_num; i++)
		{
			child = g_slist_nth_data(nffixed->children, i);

			if(child == NULL)	continue;

			nfui_nfobject_destroy(child);
		}
		g_slist_free(nffixed->children);
		nffixed->children = NULL;

	}

	nffixed = NULL;
}

void nfui_nffixed_put(NFFIXED *nffixed, NFOBJECT *child, guint x, guint y)
{
	if(nffixed->object.type != NFOBJECT_TYPE_NFFIXED && 
		nffixed->object.type != NFOBJECT_TYPE_NFCALENDAR && 
		nffixed->object.type != NFOBJECT_TYPE_NFCOMBOBOX && 
		nffixed->object.type != NFOBJECT_TYPE_NFLISTBOX && 
		nffixed->object.type != NFOBJECT_TYPE_NFSPINBUTTON && 
		nffixed->object.type != NFOBJECT_TYPE_NFTABLE && 
		nffixed->object.type != NFOBJECT_TYPE_NFTIMESPIN &&
		nffixed->object.type != NFOBJECT_TYPE_NFLOGLABEL && 
		nffixed->object.type != NFOBJECT_TYPE_NFTHUMBNAIL &&	
		nffixed->object.type != NFOBJECT_TYPE_NFTIMELABEL) 
	{
		return;
	}

	child->x = x;
	child->y = y;

	child->parent = (gpointer)nffixed;

// SKSHIN
	{
		guint i, child_num;
		NFOBJECT *p;

		child_num = g_slist_length(nffixed->children);

		for(i=0; i<child_num; i++)
		{
			p = g_slist_nth_data(nffixed->children, i);

			if(p == child)	return;
		}
	}

	nffixed->children = g_slist_append(nffixed->children, child);

	if (nfui_nfobject_is_scrolledfixed_child(nffixed))
	{
		NFSCROLLEDFIXED *scrolled_fixed;

		scrolled_fixed = nfui_nfobject_get_scrolledfixed(nffixed);
		_nfui_nfscrolledfixed_put_keylist(scrolled_fixed, child);
	}	
}

void nfui_nffixed_move(NFFIXED *nffixed, NFOBJECT *child, guint x, guint y)
{	
	if(nffixed->object.type != NFOBJECT_TYPE_NFFIXED && 
		nffixed->object.type != NFOBJECT_TYPE_NFCALENDAR && 
		nffixed->object.type != NFOBJECT_TYPE_NFCOMBOBOX && 
		nffixed->object.type != NFOBJECT_TYPE_NFLISTBOX && 
		nffixed->object.type != NFOBJECT_TYPE_NFSPINBUTTON && 
		nffixed->object.type != NFOBJECT_TYPE_NFTABLE && 
		nffixed->object.type != NFOBJECT_TYPE_NFTIMESPIN &&
		nffixed->object.type != NFOBJECT_TYPE_NFLOGLABEL && 		
		nffixed->object.type != NFOBJECT_TYPE_NFTHUMBNAIL && 
		nffixed->object.type != NFOBJECT_TYPE_NFTIMELABEL) 
	{
		return;
	}

	child->x = x;
	child->y = y;
	
	sleep(0);
}

void nfui_nffixed_set_drawing_outline(NFFIXED *nffixed, gboolean draw_outline)
{
	g_return_if_fail(nffixed != NULL);
	
	if(nffixed->object.type != NFOBJECT_TYPE_NFFIXED)
	{
		return;
	}

	if(nffixed->draw_outline != draw_outline)
		nffixed->draw_outline = draw_outline;
}



