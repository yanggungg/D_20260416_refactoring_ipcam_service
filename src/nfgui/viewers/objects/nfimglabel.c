


#include "../../support/util.h"
#include "../../support/color.h"
#include "nfimglabel.h"
#include "ix_mem.h"


#define	IMGLABEL_BORDER_SIZE	2 

static void nfimglabel_draw_outlines(NFOBJECT* obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(146));

	gdk_gc_set_line_attributes(line_gc,
			IMGLABEL_BORDER_SIZE,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = IMGLABEL_BORDER_SIZE - 1;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);
}

static void nfimglabel_erase_outlines(NFOBJECT* obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;
	gint c_idx;

	drawable = nfui_nfobject_get_window(obj);
	if(!drawable) return;

	/* outline */
	c_idx = obj->bg_color[NFOBJECT_STATE_NORMAL];
	if(c_idx < 0) return;

	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(c_idx));

	gdk_gc_set_line_attributes(line_gc,
			IMGLABEL_BORDER_SIZE,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = IMGLABEL_BORDER_SIZE - 1;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);
}

static gboolean nfimglabel_event_handler(NFIMGLABEL *nfimglabel, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;

	switch(event->type)
	{
		case GDK_EXPOSE :
		{
			guint left, top;
			gint col_idx;
			gchar *key = NULL;
			gchar key_temp[NFIMGLABEL_CASHING_KEY_SIZE];

			drawable = nfui_nfobject_get_window((NFOBJECT*)nfimglabel);
			gc = nfui_nfobject_get_gc((NFOBJECT*)nfimglabel);

			if(nfimglabel->use_cashing)
			{
				if(strlen(nfimglabel->cashing_key))
				{
					key = nfimglabel->cashing_key;
				}
				else
				{
					memset(key_temp, 0, sizeof(key_temp));
					g_sprintf(key_temp, "BG_COL_IDX_%d", col_idx);

					key = key_temp;
				}
			}

			if(nfimglabel->valid_cnt < 0)
            {
				gint pm_w, pm_h;

                nfui_get_pixbuf_size(nfimglabel->pixbuf, &pm_w, &pm_h);			
				nfimglabel->valid_cnt = nfutil_string_get_valid_count(1, drawable, nfimglabel->pango_font, nfimglabel->strLabel, nfimglabel->object.width-6-pm_w);
            }

#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
			if(nfimglabel->valid_cnt == 0)
				nfui_nfobject_use_tooltip((NFOBJECT*)nfimglabel, FALSE);
#endif

			col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfimglabel, nfimglabel->object.status);

			nfui_nfobject_get_offset((NFOBJECT*)nfimglabel, &left, &top);

			if (nfimglabel->img_align == NFALIGN_LEFT)
			{
				gint pm_w, pm_h;

//				gdk_drawable_get_size(nfimglabel->pixmap, &pm_w, &pm_h);
				nfui_get_pixbuf_size(nfimglabel->pixbuf, &pm_w, &pm_h);

			
				nfutil_draw_pixbuf(drawable, gc, nfimglabel->pixbuf, left, top, nfimglabel->object.width, nfimglabel->object.height, NFALIGN_LEFT, 2);

				if(nfimglabel->use_strip)
				{
					if(nfui_nfobject_is_supported_multi_lang(nfimglabel))
						nfutil_draw_short_text(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, nfimglabel->valid_cnt, left+pm_w+4, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_LEFT, 4, nfimglabel->shadow, NORMAL_SPACING);
					else
						nfutil_draw_short_text_eng(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, nfimglabel->valid_cnt, left+pm_w+4, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_LEFT, 4, nfimglabel->shadow, NORMAL_SPACING);
				}
				else
				{
					if(nfui_nfobject_is_supported_multi_lang(nfimglabel))
						nfutil_draw_short_text(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, 0, left+pm_w+4, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_LEFT, 4, nfimglabel->shadow, NORMAL_SPACING);
					else
						nfutil_draw_short_text_eng(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, 0, left+pm_w+4, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_LEFT, 4, nfimglabel->shadow, NORMAL_SPACING);
					//nfutil_draw_text_with_pango(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, left+pm_w+4, top, nfimglabel->object.width, nfimglabel->object.height,  nfimglabel->pango_font, &colors[nfimglabel->fg_color_idx], NFALIGN_LEFT, 0);
				}
			}
			else
			{
				nfutil_draw_pixbuf(drawable, gc, nfimglabel->pixbuf, left, top, nfimglabel->object.width, nfimglabel->object.height, NFALIGN_CENTER_LEFT, 2);

				if(nfimglabel->use_strip)
				{
					if(nfui_nfobject_is_supported_multi_lang(nfimglabel))
						nfutil_draw_short_text(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, nfimglabel->valid_cnt, left, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_CENTER_RIGHT, 4, nfimglabel->shadow, NORMAL_SPACING);
					else
						nfutil_draw_short_text_eng(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, nfimglabel->valid_cnt, left, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_CENTER_RIGHT, 4, nfimglabel->shadow, NORMAL_SPACING);
				}
				else
				{
					if(nfui_nfobject_is_supported_multi_lang(nfimglabel))
						nfutil_draw_short_text(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, 0, left, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_CENTER_RIGHT, 4, nfimglabel->shadow, NORMAL_SPACING);
					else
						nfutil_draw_short_text_eng(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, 0, left, top, nfimglabel->object.width, nfimglabel->object.height, nfimglabel->pango_font, &UX_COLOR(nfimglabel->fg_color_idx), NULL, NFALIGN_CENTER_RIGHT, 4, nfimglabel->shadow, NORMAL_SPACING);
					//nfutil_draw_text_with_pango(key, NULL, NULL, drawable, gc, nfimglabel->strLabel, left, top, nfimglabel->object.width, nfimglabel->object.height,  nfimglabel->pango_font, &colors[nfimglabel->fg_color_idx], NFALIGN_CENTER_RIGHT, 0);
				}
			}

			if(nfimglabel->draw_outline) {
				if(nfimglabel->object.kfocus == NFOBJECT_FOCUS) 
					nfimglabel_draw_outlines((NFOBJECT*)nfimglabel);
				else
					nfimglabel_erase_outlines((NFOBJECT*)nfimglabel);
			}
		
			nfui_nfobject_gc_unref(gc);
		}
		break;

		case GDK_LEAVE_NOTIFY:
			if(nfimglabel->draw_outline && nfui_nfobject_is_shown((NFOBJECT*)nfimglabel))  {
				if(nfimglabel->object.kfocus == NFOBJECT_FOCUS) 
					nfimglabel_draw_outlines((NFOBJECT*)nfimglabel);
				else
					nfimglabel_erase_outlines((NFOBJECT*)nfimglabel);
			}
			break;

		case GDK_2BUTTON_PRESS :
			break;

		case GDK_BUTTON_PRESS :
			if(nfimglabel->draw_outline && nfui_nfobject_is_shown((NFOBJECT*)nfimglabel))  {
				if(nfimglabel->object.kfocus == NFOBJECT_FOCUS) 
					nfimglabel_draw_outlines((NFOBJECT*)nfimglabel);
			}
			break;

		case GDK_BUTTON_RELEASE :
			break;

		case GDK_DELETE :
			break;

		default :
			break;
	}

	return FALSE;
}



NFIMGLABEL* nfui_nfimglabel_new(GdkPixbuf *pmImage, const gchar *strLabel)
{
	NFIMGLABEL *nfimglabel;

	nfimglabel = (NFIMGLABEL*)imalloc(sizeof(NFIMGLABEL));
	if(nfimglabel == NULL)	return NULL;

	nfimglabel->pixbuf = pmImage;
	g_stpcpy(nfimglabel->strLabel, strLabel);
	nfimglabel->fg_color_idx = COLOR_IDX(139);


#if 0
	nfimglabel->object.parent = NULL;
	nfimglabel->object.x = 0;
	nfimglabel->object.y = 0;
	nfimglabel->object.width = 100;
	nfimglabel->object.height = 30;
	nfimglabel->object.type = NFOBJECT_TYPE_NFIMGLABEL;
	nfimglabel->object.show = NFOBJECT_HIDE;
	nfimglabel->object.use_focus = NFOBJECT_FOCUS_OFF;
	nfimglabel->object.kfocus = NFOBJECT_UNFOCUS;
	nfimglabel->object.mfocus = NFOBJECT_UNFOCUS;
	nfimglabel->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	nfimglabel->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	nfimglabel->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	nfimglabel->object.bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;
	nfimglabel->object.pre_event_handler = NULL;
	nfimglabel->object.default_event_handler = nfimglabel_event_handler;
	nfimglabel->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)nfimglabel);

	nfimglabel->object.width = 100;
	nfimglabel->object.height = 30;
	nfimglabel->object.type = NFOBJECT_TYPE_NFIMGLABEL;
	nfimglabel->object.use_focus = NFOBJECT_FOCUS_ON;	//OFF;
	nfimglabel->object.default_event_handler = nfimglabel_event_handler;
	nfimglabel->spacing_type = NORMAL_SPACING;

	nfimglabel->object.use_tooltip = 1;
	nfimglabel->use_strip = 1;
	nfimglabel->draw_outline = TRUE;
#endif
	nfimglabel->valid_cnt = -1;
	nfimglabel->shadow = 0;

	nfimglabel->img_align = NFALIGN_CENTER;	

	strcpy(nfimglabel->font_name, "Calibri 16");
	nfimglabel->pango_font = nfimglabel->font_name;

	if(strlen(strLabel))
		nfui_nfobject_set_tooltip(nfimglabel, strLabel);

	return nfimglabel;
}

void nfui_nfimglabel_destroy(NFIMGLABEL *image)
{
}

void nfui_nfimglabel_set_text(NFIMGLABEL *nfimglabel, gchar *strLabel)
{
	gint length;

	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return;
	}

	memset(nfimglabel->strLabel, 0, sizeof(nfimglabel->strLabel));
	length = strlen(strLabel);

	if(length > 0)
		strncpy(nfimglabel->strLabel, strLabel, sizeof(gchar)*length);

	if(nfimglabel->object.use_tooltip)
		nfui_nfobject_set_tooltip((NFOBJECT*)nfimglabel, strLabel);
		
	nfimglabel->valid_cnt = -1;
}

void nfui_nfimglabel_set_pango_font(NFIMGLABEL *nfimglabel, const gchar *pfont, guint fg_idx)
{
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return;
	}

//	strncpy(nfimglabel->pango_font, pfont, sizeof(nfimglabel->pango_font));
	if (pfont) {
		if (nffont_is_system_font(pfont))
			nfimglabel->pango_font = pfont;
		else {
			strncpy(nfimglabel->font_name, pfont, sizeof(nfimglabel->font_name));
			nfimglabel->pango_font = nfimglabel->font_name;
		}
	}
	nfimglabel->fg_color_idx = fg_idx;
}

void nfui_nfimglabel_use_pango_cashing(NFIMGLABEL *nfimglabel, gint cashing, gchar *key)
{
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return;
	}

	if(cashing)	nfimglabel->use_cashing = 1;
	else		nfimglabel->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(nfimglabel->cashing_key, key, sizeof(nfimglabel->cashing_key));
	else
		memset(nfimglabel->cashing_key, 0, sizeof(nfimglabel->cashing_key));
}




void nfui_nfimglabel_set_spacing(NFIMGLABEL *nfimglabel, nfutil_pango_spacing_type spacing_type)
{
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return;
	}
	
	nfimglabel->spacing_type = spacing_type;
}

#if 0
void nfui_nfimglabel_set_drawing_outline(NFIMGLABEL *nfimglabel, gboolean draw_outline)
{
	g_return_if_fail(nfimglabel != NULL);
	
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return;
	}

	if(nfimglabel->draw_outline != draw_outline)
		nfimglabel->draw_outline = draw_outline;

}
#endif

void nfui_nfimglabel_use_strip(NFIMGLABEL* nfimglabel, gboolean use)
{
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return;
	}

	if(use)	nfimglabel->use_strip = 1;
	else	nfimglabel->use_strip = 0;
}



void nfui_nfimglabel_set_align(NFIMGLABEL* nfimglabel, nfalign_type align)
{
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return;
	}

	nfimglabel->img_align = align;
}

gchar* nfui_nfimglabel_get_text(NFIMGLABEL *nfimglabel)
{
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return NULL;
	}

	return nfimglabel->strLabel;
}


void nfui_nfimglabel_set_shadow(NFIMGLABEL *nfimglabel, gboolean set)
{
	if(nfimglabel->object.type != NFOBJECT_TYPE_NFIMGLABEL)
	{
		return NULL;
	}

	if(set)		nfimglabel->shadow = 1;
	else		nfimglabel->shadow = 0;
}

