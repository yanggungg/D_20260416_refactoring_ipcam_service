

/*****************************************************************************************
 *
 *
 *	NFLABEL Related...
 *
 *
 * **************************************************************************************/

#include "../../support/util.h"
#include "../../support/color.h"
#include "../../support/event_loop.h"
#include "nflabel.h"
#include "ix_mem.h"

#define LABEL_LINE_BORDER				2

static void nflabel_draw_outlines(NFOBJECT *obj)
{
	NFLABEL* nflabel;
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;

	if (((NFLABEL *)obj)->style & NFSTY_NOOUTLINE) return;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(146));

	gdk_gc_set_line_attributes(line_gc,
			LABEL_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = LABEL_LINE_BORDER - 1;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));
	nfui_nfobject_gc_unref(line_gc);

}

static void nflabel_erase_outlines(NFOBJECT *obj)
{
	NFLABEL* nflabel;
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;
	gint c_idx;

	if (((NFLABEL *)obj)->style & NFSTY_NOOUTLINE) return;

	drawable = nfui_nfobject_get_window(obj);
	if(!drawable) return;

	/* outline */
	c_idx = obj->bg_color[obj->status];
	if(c_idx < 0) return;

	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(c_idx));

	gdk_gc_set_line_attributes(line_gc,
			LABEL_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = LABEL_LINE_BORDER - 1;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));
	nfui_nfobject_gc_unref(line_gc);

}

static gboolean nflabel_event_handler(NFLABEL *nflabel, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;

	switch(event->type)
	{
		case GDK_EXPOSE :
			{
				guint left, top;
				gint height;
				GdkPixmap *pm = NULL;
				gint bg_col_idx;
				gint fg_col_idx;
				GdkGC *pm_gc;
				gchar *key = NULL;
				gchar key_temp[NFLABEL_CASHING_KEY_SIZE];
				gchar *label_string=NULL;;
				gchar strTemp[NFLABEL_MAX_STRING_SIZE];
				gchar buffer_string[NFLABEL_MAX_STRING_SIZE];

				drawable = nfui_nfobject_get_window((NFOBJECT*)nflabel);
				pm = gdk_pixmap_new(drawable, nflabel->object.width, nflabel->object.height, -1);
				gc = nfui_nfobject_get_gc((NFOBJECT*)nflabel);
				pm_gc = gdk_gc_new(pm);

				if(nfui_nfobject_is_disabled((NFOBJECT*)nflabel))
				{
					bg_col_idx = ((NFOBJECT*)nflabel)->bg_color[NFOBJECT_STATE_DISABLE];
					fg_col_idx = nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE];
				}
				else
				{
//					bg_col_idx = -1;
  //                    fg_col_idx = nflabel->fg_color_idx;
/*					if (nflabel->style & NFSTY_TRANSPARENT_BG) {
						bg_col_idx = -1;
                        fg_col_idx = nflabel->fg_color_idx;
					}
					else {*/
//if (nflabel->object.bg_color[NFOBJECT_STATE_NORMAL] != -1) {						
						bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL);
                        fg_col_idx = nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL];
//}
//else {
//						bg_col_idx = -1;
//						fg_col_idx = nflabel->fg_color_idx;
//}
//					}
				}

				if(nflabel->use_cashing)
				{
					if(strlen(nflabel->cashing_key))
					{
						key = nflabel->cashing_key;
					}
					else
					{
						memset(key_temp, 0, sizeof(key_temp));
						g_sprintf(key_temp, "BG_COL_IDX_%d", bg_col_idx);

						key = key_temp;
					}
				}


				memset(strTemp, 0, sizeof(strTemp));
				memset(buffer_string, 0x00, sizeof(buffer_string));

				if(nflabel->useNumber)
				{
					g_sprintf(strTemp, "%d", nflabel->intLabel);
					label_string = strTemp;
					strncpy(buffer_string, label_string, strlen(label_string));				
				}
				else if(nflabel->invisible)
				{
					gint i;
					gint cnt;

					cnt = strlen(nflabel->strLabel);

					for(i=0; i<cnt; i++)
						strTemp[i] = '*';

					label_string = strTemp;
					strncpy(buffer_string, label_string, strlen(label_string));
				}
				else
				{
					label_string = nflabel->strLabel;


					if(nflabel->multi_line_type == TRUE)
					{
						guint char_w = 0, string_w = 0, max_char_cnt = 0, string_cnt = 0;
						guint i = 0;

						g_assert(GDK_IS_DRAWABLE(pm));
						char_w = nfutil_string_width(0, pm, nflabel->pango_font, "@", NORMAL_SPACING);
						string_w = nfutil_string_width(nfui_nfobject_is_supported_multi_lang(nflabel), pm, nflabel->pango_font, nflabel->strLabel, NORMAL_SPACING);
						max_char_cnt = (nflabel->object.width-nflabel->margin)/char_w;
						string_cnt = strlen(nflabel->strLabel);

						if(string_w > (nflabel->object.width-nflabel->margin))
						{
							for(i=0; i<max_char_cnt; i++)
							{
								buffer_string[i] = label_string[i];
							}
							buffer_string[i] = '\n';

							for(i=max_char_cnt; i<string_cnt; i++)
							{
								buffer_string[i+1] = label_string[i];
							}
							buffer_string[i+1] = '\0';
						}
						else
						{
							strncpy(buffer_string, label_string, strlen(label_string));
						}
					}
					else
					{
						strncpy(buffer_string, label_string, strlen(label_string));
					}				
				}

#if 1
				if(bg_col_idx >= 0)
				{
//					gdk_gc_set_rgb_fg_color(pm_gc, &colors[bg_col_idx]);
//					gdk_draw_rectangle(pm, pm_gc, TRUE, 0, 0, nflabel->object.width, nflabel->object.height);
					nfui_nfobject_get_offset((NFOBJECT*)nflabel, &left, &top);
					gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
					gdk_draw_rectangle(drawable, gc, TRUE, left, top, nflabel->object.width, nflabel->object.height);
				}
#endif


			if(nflabel->use_strip)
			{
				if(nflabel->valid_cnt < 0)
				{
					if(nflabel->is_vk) {
						memset(strTemp, 0, sizeof(strTemp));
						strcpy(strTemp, nflabel->strLabel);
					}

					if(nflabel->is_vk)
						nflabel->valid_cnt = nfutil_string_get_valid_count(nfui_nfobject_is_supported_multi_lang(nflabel), drawable, nflabel->pango_font, g_strreverse(strTemp), (nflabel->object.width-nflabel->margin));
					else
						nflabel->valid_cnt = nfutil_string_get_valid_count(nfui_nfobject_is_supported_multi_lang(nflabel), drawable, nflabel->pango_font, nflabel->strLabel, (nflabel->object.width-nflabel->margin));
#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
					if(nflabel->valid_cnt == 0)
						nfui_nfobject_use_tooltip((NFOBJECT*)nflabel, FALSE);
#endif
				}

			}
			else
			{
				nflabel->valid_cnt = 0;
			}

			height = nflabel->object.height;

			nfui_nfobject_get_offset((NFOBJECT*)nflabel, &left, &top);


			if(nflabel->is_vk)
			{ 
				if(nfui_nfobject_is_supported_multi_lang(nflabel))
					nfutil_draw_short_text_vk(key, NULL, (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), drawable, gc, buffer_string, nflabel->valid_cnt, left, top, nflabel->object.width, height, nflabel->pango_font, &UX_COLOR(fg_col_idx), NULL, nflabel->align, nflabel->margin, nflabel->shadow, nflabel->spacing_type);
				else
					nfutil_draw_short_text_eng_vk(key, NULL, (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), drawable, gc, buffer_string, nflabel->valid_cnt, left, top, nflabel->object.width, height, nflabel->pango_font, &UX_COLOR(fg_col_idx), NULL, nflabel->align, nflabel->margin, nflabel->shadow, nflabel->spacing_type);
			}
			else
			{
				if(nfui_nfobject_is_supported_multi_lang(nflabel))
					nfutil_draw_short_text(key, NULL, (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), drawable, gc, buffer_string, nflabel->valid_cnt, left, top, nflabel->object.width, height, nflabel->pango_font, &UX_COLOR(fg_col_idx), NULL, nflabel->align, nflabel->margin, nflabel->shadow, nflabel->spacing_type);
				else
					nfutil_draw_short_text_eng(key, NULL, (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), drawable, gc, buffer_string, nflabel->valid_cnt, left, top, nflabel->object.width, height, nflabel->pango_font, &UX_COLOR(fg_col_idx), NULL, nflabel->align, nflabel->margin, nflabel->shadow, nflabel->spacing_type);
			}


//			gdk_draw_drawable(drawable, gc, pm, 0, 0, left, top, nflabel->object.width, nflabel->object.height);
			nfui_nfobject_gc_unref(gc);
			g_object_unref(pm_gc);
			g_object_unref(pm);

			if(nflabel->object.use_focus) {
				if(nflabel->draw_outline) { 
					if(nflabel->object.kfocus == NFOBJECT_FOCUS)
						nflabel_draw_outlines((NFOBJECT*)nflabel);
					else
						nflabel_erase_outlines((NFOBJECT*)nflabel);
				}
			}

			}
			break;

		case GDK_LEAVE_NOTIFY:
			if(nflabel->object.use_focus && nflabel->draw_outline) {
				if(nfui_nfobject_is_shown((NFOBJECT*)nflabel)) {
					if(nflabel->object.kfocus == NFOBJECT_FOCUS)
						nflabel_draw_outlines((NFOBJECT*)nflabel);
					else
						nflabel_erase_outlines((NFOBJECT*)nflabel);
				}
			}
			break;

		case GDK_2BUTTON_PRESS :
			break;

		case GDK_BUTTON_PRESS :
			if(nflabel->object.use_focus && nflabel->draw_outline) {
				if(nfui_nfobject_is_shown((NFOBJECT*)nflabel)) {
					if(nflabel->object.kfocus == NFOBJECT_FOCUS)
						nflabel_draw_outlines((NFOBJECT*)nflabel);
				}
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

NFLABEL* nfui_nflabel_new(gchar *strLabel)
{
	NFLABEL *nflabel;
	gint length;

	nflabel = (NFLABEL*)imalloc(sizeof(NFLABEL));
	if(nflabel == NULL)	return NULL;
	
#if 0
	nflabel->object.parent = NULL;
	nflabel->object.x = 0;
	nflabel->object.y = 0;
	nflabel->object.width = 100;
	nflabel->object.height = 22;
	nflabel->object.type = NFOBJECT_TYPE_NFLABEL;
	nflabel->object.show = NFOBJECT_HIDE;
	nflabel->object.use_focus = NFOBJECT_FOCUS_ON;	//OFF;
	nflabel->object.kfocus = NFOBJECT_UNFOCUS;
	nflabel->object.mfocus = NFOBJECT_UNFOCUS;
	nflabel->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	nflabel->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	nflabel->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	nflabel->object.bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;

	nflabel->object.pre_event_handler = NULL;
	nflabel->object.default_event_handler = nflabel_event_handler;
	nflabel->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)nflabel);

	nflabel->object.width = 100;
	nflabel->object.height = 22;
	nflabel->object.type = NFOBJECT_TYPE_NFLABEL;
	nflabel->object.use_focus = NFOBJECT_FOCUS_ON;	//OFF;
	nflabel->object.default_event_handler = nflabel_event_handler;
	
	nflabel->object.use_tooltip = 1;
#endif

	nflabel->valid_cnt = -1;
	nflabel->use_strip = 1;
	nflabel->use_cashing = 1;
	nflabel->shadow = 0;
       nflabel->margin = 0;

	length = strlen(strLabel);

	if(length)
	{
		strncpy(nflabel->strLabel, strLabel, sizeof(gchar)*length);
		nfui_nfobject_set_tooltip((NFOBJECT*)nflabel, strLabel);
	}

	nflabel->useNumber = 0;
	nflabel->intLabel = 0;

	g_sprintf(nflabel->font_name, "Arial bold 19");
	nflabel->pango_font = nflabel->font_name;

	nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = COLOR_IDX(129);
	nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(131);
	nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(133);
	nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = COLOR_IDX(135);			
	nflabel->align = NFALIGN_CENTER;

	nflabel->draw_outline = TRUE;	
	nflabel->invisible = FALSE;	

	nflabel->spacing_type = NORMAL_SPACING;
	nflabel->multi_line_type = FALSE;

	nflabel->is_vk = FALSE;
 	nflabel->skin_type = NFTEXTBOX_TYPE_UNUSED;
  
	return nflabel;
}



void nfui_nflabel_destroy(NFLABEL *nflabel)
{
	
}

void nfui_nflabel_set_text(NFLABEL *nflabel, gchar *strLabel)
{
	gint length;

	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	memset(nflabel->strLabel, 0, sizeof(nflabel->strLabel));
	length = strlen(strLabel);

	if(length > 0)
		strncpy(nflabel->strLabel, strLabel, sizeof(gchar)*length);
#if 0
	if(nflabel->object.use_tooltip)
	{
		nfui_nfobject_set_tooltip((NFOBJECT*)nflabel, strLabel);
		nflabel->valid_cnt = -1;
	}
#endif
	
	nflabel->valid_cnt = -1;
	if (!nflabel->invisible)
		nfui_nfobject_set_tooltip((NFOBJECT*)nflabel, strLabel);

	if(!nflabel->use_strip)
		((NFOBJECT*)nflabel)->use_tooltip = 0;
}

gchar* nfui_nflabel_get_text(NFLABEL *nflabel)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return NULL;
	}

	return nflabel->strLabel;
}

int nfui_nflabel_get_text_b(NFLABEL *nflabel, char *buf, int len)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return NULL;
	}

	strncpy(buf, nflabel->strLabel, len);
	return 0;
}

void nfui_nflabel_use_number(NFLABEL *nflabel, gboolean use)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	if(use)	nflabel->useNumber = 1;
	else	nflabel->useNumber = 0;
}

void nfui_nflabel_set_number(NFLABEL *nflabel, gint num)
{
	gchar strTemp[256];

	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	nflabel->intLabel = num;

	if(nflabel->object.use_tooltip)
	{
		memset(strTemp, 0, sizeof(strTemp));
		g_sprintf(strTemp, "%d", num);

		nfui_nfobject_set_tooltip((NFOBJECT*)nflabel, strTemp);
		nflabel->valid_cnt = -1;
	}
}

gint nfui_nflabel_get_number(NFLABEL *nflabel)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return 0;
	}

	return nflabel->intLabel;
}

gint nfui_nflabel_get_strlen(NFLABEL *nflabel)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return 0;
	}

	return strlen(nflabel->strLabel);
}

void nfui_nflabel_set_align(NFLABEL *nflabel, nfalign_type align, guint margin)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	nflabel->align = align;
	nflabel->margin = margin;
}

void nfui_nflabel_modify_fg(NFLABEL *nflabel, guint fg_idx)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = fg_idx;
}

void nfui_nflabel_set_shadow(NFLABEL *nflabel, gboolean set)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	if(set)		nflabel->shadow = 1;
	else		nflabel->shadow = 0;
}

void nfui_nflabel_set_drawing_outline(NFLABEL *nflabel, gboolean draw_outline)
{
	g_return_if_fail(nflabel != NULL);
	
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	if(nflabel->draw_outline != draw_outline)
		nflabel->draw_outline = draw_outline;
}


void nfui_nflabel_set_invisible(NFLABEL *nflabel, gboolean invisible)
{
	g_return_if_fail(nflabel != NULL);
	
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

		nflabel->invisible = invisible;

	if(invisible)	nfui_nfobject_use_tooltip((NFOBJECT*)nflabel, FALSE);
	else			nfui_nfobject_use_tooltip((NFOBJECT*)nflabel, TRUE);
}

gboolean nfui_nflabel_get_invisible(NFLABEL *nflabel)
{
	g_return_if_fail(nflabel != NULL);

	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	return nflabel->invisible;
}

NFLABEL* nfui_nflabel_new_with_pango_font(gchar *strLabel, gchar *pfont, guint fg_idx)
{
	NFLABEL *nflabel;

	nflabel = nfui_nflabel_new(strLabel);

//	strncpy(nflabel->pango_font, pfont, sizeof(nflabel->pango_font));
	if (pfont) {
		if (nffont_is_system_font(pfont))
			nflabel->pango_font = pfont;
		else {
			strncpy(nflabel->font_name, pfont, sizeof(nflabel->font_name));
			nflabel->pango_font = nflabel->font_name;
		}
	}
	nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = fg_idx;

	return nflabel;
}

NFLABEL* nfui_nflabel_new_with_pango_font_and_vk(gchar *strLabel, gchar *pfont, guint fg_idx)
{
	NFLABEL *nflabel;

	nflabel = nfui_nflabel_new(strLabel);

//	strncpy(nflabel->pango_font, pfont, sizeof(nflabel->pango_font));
	if (pfont) {
		if (nffont_is_system_font(pfont))
			nflabel->pango_font = pfont;
		else {
			strncpy(nflabel->font_name, pfont, sizeof(nflabel->font_name));
			nflabel->pango_font = nflabel->font_name;
		}
	}
	nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = fg_idx;

	nflabel->is_vk = TRUE;

	return nflabel;
}


void nfui_nflabel_set_pango_font(NFLABEL *nflabel, gchar *pfont, guint fg_idx)
{
	g_return_if_fail(nflabel != NULL);
	
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

//	if(pfont) 
//		strncpy(nflabel->pango_font, pfont, sizeof(nflabel->pango_font));

	if (pfont) {
		if (nffont_is_system_font(pfont))
			nflabel->pango_font = pfont;
		else {
			strncpy(nflabel->font_name, pfont, sizeof(nflabel->font_name));
			nflabel->pango_font = nflabel->font_name;
		}
	}

	nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = fg_idx;
}

void nfui_nflabel_use_pango_cashing(NFLABEL *nflabel, gint cashing, gchar *key)
{
	g_return_if_fail(nflabel != NULL);
	
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	if(cashing)	nflabel->use_cashing = 1;
	else		nflabel->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(nflabel->cashing_key, key, sizeof(nflabel->cashing_key));
	else
		memset(nflabel->cashing_key, 0, sizeof(nflabel->cashing_key));
}


void nfui_nflabel_set_spacing(NFLABEL *nflabel, nfutil_pango_spacing_type spacing_type)
{
	g_return_if_fail(nflabel != NULL);
	
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

  nflabel->spacing_type = spacing_type;
}


void nfui_nflabel_set_multi_line_type(NFLABEL *nflabel, gboolean multi_line_type)
{
	g_return_if_fail(nflabel != NULL);
	
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

  nflabel->multi_line_type = multi_line_type;
}

void nfui_nflabel_use_strip(NFLABEL* nflabel, gboolean use)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	if(use)	nflabel->use_strip = 1;
	else	nflabel->use_strip = 0;
}

void nfui_nflabel_set_style(NFLABEL* nflabel, NFSTYLE style)
{
	((NFLABEL *)nflabel)->style |= style;
}

guint nfui_nflabel_get_pango_font_color(NFLABEL *nflabel)
{
	return nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL];
}




NFLABEL* nfui_nflabel_new_text_box(gchar *strLabel, gchar *pfont)
{
	NFLABEL *nflabel;

	nflabel = nfui_nflabel_new(strLabel);
//	strncpy(nflabel->pango_font, pfont, sizeof(nflabel->pango_font));
	if (pfont) {
		if (nffont_is_system_font(pfont))
			nflabel->pango_font = pfont;
		else {
			strncpy(nflabel->font_name, pfont, sizeof(nflabel->font_name));
			nflabel->pango_font = nflabel->font_name;
		}
	}

	nflabel->skin_type = NFTEXTBOX_TYPE_UNDEF;
	return nflabel;
}

void nfui_nflabel_set_skin_type(NFLABEL* nflabel, NFTEXTBOX_TYPE type)
{
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	if (type == NFTEXTBOX_TYPE_INPUT)
	{
		nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = COLOR_IDX(129);
		nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(131);
		nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(133);
		nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = COLOR_IDX(135);		
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));		
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_DISABLE, COLOR_IDX(134));		
		nfui_nfobject_support_multi_lang(nflabel, FALSE);

		nflabel->skin_type = type;
	}
	else if (type == NFTEXTBOX_TYPE_OUTPUT)
	{
		nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = COLOR_IDX(139);
		nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(141);
		nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(143);
		nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = COLOR_IDX(145);				
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(138));		
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_DISABLE, COLOR_IDX(144));		
		nflabel->skin_type = type;
	}
	else if (type == NFTEXTBOX_TYPE_POPUP_INPUT)
	{
		nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = COLOR_IDX(221);
		nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(223);
		nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(225);
		nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = COLOR_IDX(227);				
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));		
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_DISABLE, COLOR_IDX(226));		
		nfui_nfobject_support_multi_lang(nflabel, FALSE);
		
		nflabel->skin_type = type;
	}
	else if (type == NFTEXTBOX_TYPE_POPUP_OUTPUT)
	{
		nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = COLOR_IDX(231);
		nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(233);
		nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(235);
		nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = COLOR_IDX(237);				
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));		
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_DISABLE, COLOR_IDX(236));		
		nflabel->skin_type = type;
	}
	else if (type == NFTEXTBOX_TYPE_SUBTAB_INPUT)
	{
		nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = COLOR_IDX(911);
		nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(913);
		nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(915);
		nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = COLOR_IDX(917);				
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));		
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_DISABLE, COLOR_IDX(916));		
		nfui_nfobject_support_multi_lang(nflabel, FALSE);
		
		nflabel->skin_type = type;
	}
	else if (type == NFTEXTBOX_TYPE_SUBTAB_OUTPUT)
	{
		nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = COLOR_IDX(921);
		nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(923);
		nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(925);
		nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = COLOR_IDX(927);				
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(920));		
		nfui_nfobject_modify_bg((NFOBJECT*)nflabel, NFOBJECT_STATE_DISABLE, COLOR_IDX(926));		
		nflabel->skin_type = type;
	}	
}

void nfui_nflabel_set_fg_color(NFLABEL *nflabel, gint *fg_color)
{
	g_return_if_fail(nflabel != NULL);
	
	if(nflabel->object.type != NFOBJECT_TYPE_NFLABEL)
	{
		return;
	}

	nflabel->fg_color_idx[NFOBJECT_STATE_NORMAL] = fg_color[NFOBJECT_STATE_NORMAL];
	nflabel->fg_color_idx[NFOBJECT_STATE_PRELIGHT] = fg_color[NFOBJECT_STATE_PRELIGHT];
	nflabel->fg_color_idx[NFOBJECT_STATE_ACTIVE] = fg_color[NFOBJECT_STATE_ACTIVE];
	nflabel->fg_color_idx[NFOBJECT_STATE_DISABLE] = fg_color[NFOBJECT_STATE_DISABLE];
}

void nfui_nflabel_erase(NFLABEL *nflabel)
{
	GdkGC *gc;
	GdkDrawable *drawable = NULL;
	gint bg_col_idx;
	int left, top;
	
	if(nfui_nfobject_is_disabled((NFOBJECT*)nflabel))
	{
		bg_col_idx = ((NFOBJECT*)nflabel)->bg_color[NFOBJECT_STATE_DISABLE];
	}
	else
	{
		bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nflabel, NFOBJECT_STATE_NORMAL);
	}

	gc = nfui_nfobject_get_gc((NFOBJECT*)nflabel);
	drawable = nfui_nfobject_get_window((NFOBJECT*)nflabel);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));

	nfui_nfobject_get_offset((NFOBJECT*)nflabel, &left, &top);
	gdk_draw_rectangle(drawable, gc, TRUE, left, top, nflabel->object.width, nflabel->object.height);

	nfui_nfobject_gc_unref(gc);
}
