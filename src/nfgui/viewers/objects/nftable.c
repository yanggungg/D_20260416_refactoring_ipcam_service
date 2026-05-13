

#include "nftable.h"
#include "nfscrolledfixed.h"

#include "../../support/nf_ui_color.h"
#include "ix_mem.h"


static void nftable_draw_outline(NFTABLE *ntb);



static void nftable_draw_outline(NFTABLE *ntb)
{
	GdkDrawable *drawable;
	GdkColor line_color = UX_COLOR(392);
	GdkGC *line_gc;
	gint pos_x, pos_y, size_w, size_h;
	gint gap = 0;
	gint i;


	drawable = nfui_nfobject_get_window((NFOBJECT*)ntb);
	line_gc = nfui_nfobject_get_gc((NFOBJECT*)ntb);
	gdk_gc_set_rgb_fg_color(line_gc, &line_color);

	gdk_gc_set_line_attributes(line_gc,
							1,
							GDK_LINE_SOLID,
							GDK_CAP_NOT_LAST,
							GDK_JOIN_BEVEL);

	nfui_nfobject_get_offset((NFOBJECT*)ntb, &pos_x, &pos_y);
	size_w = ((NFOBJECT*)ntb)->width;
	size_h = ((NFOBJECT*)ntb)->height;

	/* outline */
	gdk_draw_rectangle(drawable,
			line_gc,
			FALSE,
			pos_x - 1, pos_y - 1,
			size_w + 2, size_h + 2);


	/* inline */
	for(i=0; i<(ntb->cols - 1); i++) {
		gap += ntb->cell_width[i];

		if(i > 0) gap += ntb->col_space;

		gdk_draw_line(drawable,
				line_gc,
				pos_x + gap, 
				pos_y,
				pos_x + gap, 
				(pos_y + size_h));
	}
	
	gap = 0;	
	for(i=0; i<(ntb->rows - 1); i++) {
		gap += ntb->cell_height;

		if(i > 0) gap += ntb->row_space;

		gdk_draw_line(drawable,
				line_gc,
				pos_x, 
				pos_y + gap,
				pos_x + size_w, 
				pos_y + gap);
	}

	nfui_nfobject_gc_unref(line_gc);
}

static gboolean nftable_event_handler(NFTABLE *ntb, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE :
			if(ntb->draw_outline)
				nftable_draw_outline(ntb);
			break;

		case GDK_DELETE :
			if(ntb && ntb->children)
				g_slist_free(ntb->children);
			ntb->children = NULL;

			break;

		default :
			break;
	}



	return FALSE;
}


NFTABLE *nfui_nftable_new(guint cols, guint rows, guint col_space, guint row_space, guint *cell_width, guint cell_height)
{
	NFTABLE *ntb;
	guint i, width_tmp;
	
	if(cols > NFTABLE_MAX_COLUMNS)
	{
		//return NULL;
		g_warning(">>>>>>>>>>>>>>>>> %s, %d col : %d", __FUNCTION__, __LINE__, cols);			
		cols = NFTABLE_MAX_COLUMNS;
	}

	if(rows > NFTABLE_MAX_ROWS)
	{
		g_warning(">>>>>>>>>>>>>>>>> %s, %d row : %d", __FUNCTION__, __LINE__, rows);	
		rows = NFTABLE_MAX_ROWS;
	}

	ntb = (NFTABLE*)imalloc(sizeof(NFTABLE));
	if(ntb == NULL)	return NULL;

	width_tmp = 0;

	ntb->children = NULL;

	ntb->cols = cols;
	ntb->rows = rows;
	ntb->col_space = col_space;
	ntb->row_space = row_space;
	ntb->cell_height = cell_height;

	ntb->draw_outline = FALSE;

	for(i=0; i<cols; i++)
	{
		ntb->cell_width[i] = cell_width[i];
		width_tmp += cell_width[i];
		width_tmp += col_space;
	}
	width_tmp -= col_space; 

#if 0
	ntb->object.parent = NULL;
	ntb->object.x = 0;
	ntb->object.y = 0;
	ntb->object.width = width_tmp;
	ntb->object.height = (cell_height+row_space) * rows - row_space;
	ntb->object.type = NFOBJECT_TYPE_NFTABLE;
	ntb->object.show = NFOBJECT_HIDE;
	ntb->object.use_focus = NFOBJECT_FOCUS_OFF;
	ntb->object.kfocus = NFOBJECT_UNFOCUS;
	ntb->object.mfocus = NFOBJECT_UNFOCUS;
	ntb->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	ntb->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	ntb->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	ntb->object.pre_event_handler = NULL;
	ntb->object.default_event_handler = nftable_event_handler;
	ntb->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)ntb);

	ntb->object.width = width_tmp;
	ntb->object.height = (cell_height+row_space) * rows - row_space;
	ntb->object.type = NFOBJECT_TYPE_NFTABLE;
	ntb->object.default_event_handler = nftable_event_handler;
#endif

	return ntb;
}

void nfui_nftable_destroy(NFTABLE *ntb)
{
}

void nfui_nftable_attach(NFTABLE *ntb, NFOBJECT *obj, guint col, guint row)
{	
	guint cx, cy;
	guint i;

	if(ntb->object.type != NFOBJECT_TYPE_NFTABLE)
	{
		return;
	}

	if(col>=ntb->cols || row >=ntb->rows)
	{
		g_printf("Check NFTABLE size:: columns[%d], rows[%d]\n", ntb->cols, ntb->rows);
		//SKSHIN
		iassert(0);
		return;
	}

	cx = 0;
	if(col>0)
	{
		for(i=0; i<col; i++)
		{
			cx += ntb->cell_width[i];
			cx += ntb->col_space;
		}
	}

	cy = (ntb->cell_height + ntb->row_space) * row;

	obj->x = cx;
	obj->y = cy;
	obj->width = ntb->cell_width[col];
	obj->height = ntb->cell_height;

	obj->parent = ntb;

	ntb->children = g_slist_append(ntb->children, obj);

//	ntb->map[row * ntb->cols + col] = g_slist_length(ntb->children) - 1;
	ntb->map[row][col] = g_slist_length(ntb->children) - 1;

	if (nfui_nfobject_is_scrolledfixed_child(ntb))
	{
		NFSCROLLEDFIXED *scrolled_fixed;

		scrolled_fixed = nfui_nfobject_get_scrolledfixed(ntb);
		_nfui_nfscrolledfixed_put_keylist(scrolled_fixed, obj);
	}
}


NFOBJECT* nfui_nftable_get_child(NFTABLE *ntb, guint col, guint row)
{
	NFOBJECT *obj;
//	guint cols, nth;

	if(ntb->object.type != NFOBJECT_TYPE_NFTABLE)
	{
		return;
	}

//	cols = ntb->cols;

//	nth = row * cols + col;
//	obj = (NFOBJECT*)g_slist_nth_data(ntb->children, ntb->map[nth]);
	obj = (NFOBJECT*)g_slist_nth_data(ntb->children, ntb->map[row][col]);

	return obj;
}

void nfui_nftable_set_draw_outline(NFTABLE *ntb, gboolean draw)
{
	if(ntb->object.type != NFOBJECT_TYPE_NFTABLE)
	{
		return;
	}

	ntb->draw_outline = draw;
}

int nfui_nftable_get_width(NFTABLE *ntb)
{
	return ntb->object.width;
}

int nfui_nftable_get_height(NFTABLE *ntb)
{
	return ntb->object.height;
}

