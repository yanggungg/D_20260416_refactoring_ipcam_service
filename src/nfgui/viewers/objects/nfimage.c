
#include "../../support/util.h"
#include "../../support/color.h"
#include "nfimage.h"
#include "ix_mem.h"
#include "../../tools/nf_ui_tool.h"


static void nfimage_draw_text(NFIMAGE *nfimage, GdkDrawable *drawable, GdkGC *gc, guint x, guint y)
{
	gchar *key = NULL;
	gchar key_temp[NFIMAGE_CASHING_KEY_SIZE];
	guint fg_color = nfimage->font_color;
	guint w, h;

	if(nfimage->valid_cnt < 0)
	{
		nfimage->valid_cnt = nfutil_string_get_valid_count(nfui_nfobject_is_supported_multi_lang(nfimage), drawable, nfimage->pango_font, nfimage->strLabel, nfimage->object.width - nfimage->font_margin);

#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
		if(nfimage->valid_cnt == 0)
			nfui_nfobject_use_tooltip((NFOBJECT*)nfimage, FALSE);
#endif
	}

	if(nfimage->use_cashing)
	{
		if(nfimage->cashing_key && strlen(nfimage->cashing_key))
		{
			key = nfimage->cashing_key;
		}
		else
		{
			memset(key_temp, 0, sizeof(key_temp));
			g_sprintf(key_temp, "PTR_IMG[%d]_%p", nfimage->object.status, nfimage->pixbuf);

			key = key_temp;
		}
	}

	w = nfimage->object.width;
	h = nfimage->object.height;

#ifndef NF_TOOLTIP_ENABLE
	if(nfimage->spacing_type == NORMAL_SPACING)
	{
		if(nfui_nfobject_is_supported_multi_lang(nfimage))
			nfutil_draw_text_with_pango(key, nfimage->pixbuf, NULL, drawable, gc, nfimage->strLabel, x, y, w, h, nfimage->pango_font, &UX_COLOR(fg_color), nfimage->font_align, nfimage->font_margin);
	else
			nfutil_draw_text_with_pango_eng(key, nfimage->pixbuf, NULL, drawable, gc, nfimage->strLabel, x, y, w, h, nfimage->pango_font, &UX_COLOR(fg_color), nfimage->font_align, nfimage->font_margin);
	}
#else
	if(nftool_cur_language_is_eng())
	{
		if(nfui_nfobject_is_supported_multi_lang(nfimage))
			nfutil_draw_short_text(key, nfimage->pixbuf, NULL, drawable, gc, nfimage->strLabel, nfimage->valid_cnt, x , y, w, h, nfimage->pango_font, &UX_COLOR(fg_color), NULL, nfimage->font_align, nfimage->font_margin, 0, nfimage->spacing_type);
		else
			nfutil_draw_short_text_eng(key, nfimage->pixbuf, NULL, drawable, gc, nfimage->strLabel, nfimage->valid_cnt, x , y, w, h, nfimage->pango_font, &UX_COLOR(fg_color), NULL, nfimage->font_align, nfimage->font_margin, 0, nfimage->spacing_type);
	}
	else
	{
		if(nfui_nfobject_is_supported_multi_lang(nfimage))
			nfutil_draw_short_text(key, nfimage->pixbuf, NULL, drawable, gc, nfimage->strLabel, nfimage->valid_cnt,x, y, w, h, nfimage->pango_font, &UX_COLOR(fg_color), NULL, nfimage->font_align, nfimage->font_margin, 0, nfimage->spacing_type);
		else
			nfutil_draw_short_text_eng(key, nfimage->pixbuf, NULL, drawable, gc, nfimage->strLabel, nfimage->valid_cnt,x, y, w, h, nfimage->pango_font, &UX_COLOR(fg_color), NULL, nfimage->font_align, nfimage->font_margin, 0, nfimage->spacing_type);
	}
#endif



}


static gboolean nfimage_event_handler(NFIMAGE *nfimage, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE :
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)nfimage);
			gc = nfui_nfobject_get_gc((NFOBJECT*)nfimage);

			nfui_nfobject_get_offset((NFOBJECT*)nfimage, &x, &y);

			if(!strlen(nfimage->strLabel))
			nfutil_draw_pixbuf(drawable, gc, nfimage->pixbuf, x, y, nfimage->object.width, nfimage->object.height, nfimage->align, nfimage->margin);

			if(nfimage->strLabel)
				nfimage_draw_text(nfimage, drawable, gc, x, y);

		}
		break;

		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
//			if(nfui_nfobject_is_shown((NFOBJECT*)nfimage)) {
				// skshin
//				nfui_signal_emit((NFOBJECT*)nfimage, GDK_EXPOSE, TRUE);
				//nfui_signal_emit((NFOBJECT*)nfimage, GDK_EXPOSE, FALSE);
//			}
			break;

		case GDK_DELETE :
			if(!nfimage->use_cashing) {
				g_object_unref(nfimage->pixbuf);
				nfimage->pixbuf = NULL;
			}
			break;

		default :
			break;
	}

	return FALSE;
}



NFIMAGE* nfui_nfimage_new(const gchar *strImage)
{
	NFIMAGE *nfimage;
	guint img_w, img_h;

	nfimage = (NFIMAGE*)imalloc(sizeof(NFIMAGE));
	if(nfimage == NULL)	return NULL;

	nfimage->pixbuf = nfui_get_image_from_file(strImage, NULL);
	if(!nfimage->pixbuf)
	{
		ifree(nfimage);
		nfimage = NULL;
		return NULL;
	}

	nfimage->align = NFALIGN_CENTER;
	nfimage->margin = 0;

g_assert(GDK_IS_PIXBUF(nfimage->pixbuf));
	nfui_get_pixbuf_size(nfimage->pixbuf, &img_w, &img_h);

#if 0
	nfimage->object.parent = NULL;
	nfimage->object.x = 0;
	nfimage->object.y = 0;
	nfimage->object.width = img_w;
	nfimage->object.height = img_h;
	nfimage->object.type = NFOBJECT_TYPE_NFIMAGE;
	nfimage->object.show = NFOBJECT_HIDE;
	nfimage->object.use_focus = NFOBJECT_FOCUS_OFF;
	nfimage->object.kfocus = NFOBJECT_UNFOCUS;
	nfimage->object.mfocus = NFOBJECT_UNFOCUS;
	nfimage->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	nfimage->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	nfimage->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	nfimage->object.pre_event_handler = NULL;
	nfimage->object.default_event_handler = nfimage_event_handler;
	nfimage->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)nfimage);

	nfimage->object.width = img_w;
	nfimage->object.height = img_h;
	nfimage->object.type = NFOBJECT_TYPE_NFIMAGE;
	nfimage->object.default_event_handler = nfimage_event_handler;

	nfimage->use_cashing = 1;
	nfimage->valid_cnt = -1;
	nfimage->font_align = NFALIGN_CENTER;
	nfimage->font_margin = 0;
	nfimage->spacing_type = NORMAL_SPACING;

	strcpy(nfimage->font_name, "Calibri 16");
	nfimage->pango_font = nfimage->font_name;
#endif

	return nfimage;
}

NFIMAGE* nfui_nfimage_new_pixbuf(GdkPixbuf *pf)
{
	NFIMAGE *nfimage;
	guint img_w, img_h;

	nfimage = (NFIMAGE*)imalloc(sizeof(NFIMAGE));
	if(nfimage == NULL)	return NULL;

	//nfimage->pixbuf = gdk_pixbuf_copy(pf);
	nfimage->pixbuf = pf;
	if(!nfimage->pixbuf)
	{
		ifree(nfimage);
		nfimage = NULL;
		return NULL;
	}

	nfimage->align = NFALIGN_CENTER;
	nfimage->margin = 0;

g_assert(GDK_IS_PIXBUF(nfimage->pixbuf));
	nfui_get_pixbuf_size(nfimage->pixbuf, &img_w, &img_h);

#if 0
	nfimage->object.parent = NULL;
	nfimage->object.x = 0;
	nfimage->object.y = 0;
	nfimage->object.width = img_w;
	nfimage->object.height = img_h;
	nfimage->object.type = NFOBJECT_TYPE_NFIMAGE;
	nfimage->object.show = NFOBJECT_HIDE;
	nfimage->object.use_focus = NFOBJECT_FOCUS_OFF;
	nfimage->object.kfocus = NFOBJECT_UNFOCUS;
	nfimage->object.mfocus = NFOBJECT_UNFOCUS;
	nfimage->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	nfimage->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	nfimage->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	nfimage->object.pre_event_handler = NULL;
	nfimage->object.default_event_handler = nfimage_event_handler;
	nfimage->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)nfimage);

	nfimage->object.width = img_w;
	nfimage->object.height = img_h;
	nfimage->object.type = NFOBJECT_TYPE_NFIMAGE;
	nfimage->object.default_event_handler = nfimage_event_handler;

	nfimage->use_cashing = 0;
	nfimage->valid_cnt = -1;
	nfimage->font_align = NFALIGN_CENTER;
	nfimage->font_margin = 0;
	nfimage->spacing_type = NORMAL_SPACING;

	strcpy(nfimage->font_name, "Calibri 16");
	nfimage->pango_font = nfimage->font_name;
#endif

	return nfimage;
}

void nfui_nfimage_destroy(NFIMAGE *image)
{
	if(image->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return;
	}

	if(image)
	{
		memset(image, 0, sizeof(NFIMAGE));
		ifree(image);
		image = NULL;
	}
}

gboolean nfui_nfimage_change_image(NFIMAGE *nfimage, const gchar *strImage)
{
	GdkPixbuf *pb;
	guint img_w, img_h;

	if(nfimage->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return FALSE;
	}

	pb = nfui_get_image_from_file(strImage, NULL);
	if(pb == nfimage->pixbuf)
		return FALSE;

	nfimage->pixbuf = pb;

g_assert(GDK_IS_PIXBUF(nfimage->pixbuf));
	nfui_get_pixbuf_size(nfimage->pixbuf, &img_w, &img_h);

	nfimage->object.width = img_w;
	nfimage->object.height = img_h;

	return TRUE;
}

void nfui_nfimage_set_text(NFIMAGE *nfimage, const gchar *strLabel)
{
	gint length;

	if(nfimage->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return;
	}

	memset(nfimage->strLabel, 0, sizeof(nfimage->strLabel));
	length = strlen(strLabel);

	if(length > 0)	strncpy(nfimage->strLabel, strLabel, sizeof(gchar)*length);

	if(nfimage->object.use_tooltip)
	{
		nfui_nfobject_set_tooltip((NFOBJECT*)nfimage, strLabel);
		nfimage->valid_cnt = -1;
	}
}

gchar* nfui_nfimage_get_text(NFIMAGE *nfimage)
{
	if(nfimage->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return;
	}

#if 1
    return nfimage->strLabel;
#else
	if(strlen(nfimage->strLabel))
		return nfimage->strLabel;
	else
		return NULL;
#endif
}

void nfui_nfimage_set_pango_font(NFIMAGE *nfimage, const gchar *pfont, guint font_color)
{
	if(nfimage->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return;
	}

//	strncpy(nfimage->pango_font, pfont, sizeof(nfimage->pango_font));
	if (pfont) {
		if (nffont_is_system_font(pfont))
			nfimage->pango_font = pfont;
		else {
			strncpy(nfimage->font_name, pfont, sizeof(nfimage->font_name));
			nfimage->pango_font = nfimage->font_name;
		}
	}
	nfimage->font_color = font_color;
}

void nfui_nfimage_use_pango_cashing(NFIMAGE *nfimage, gint cashing, gchar *key)
{
	g_return_if_fail(nfimage != NULL);

	if(nfimage->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return;
	}

	if(cashing)	nfimage->use_cashing = 1;
	else		nfimage->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(nfimage->cashing_key, key, sizeof(nfimage->cashing_key));
	else
		memset(nfimage->cashing_key, 0, sizeof(nfimage->cashing_key));
}

void nfui_nfimage_set_font_alignment(NFIMAGE *nfimage, nfalign_type align, guint margin)
{
	g_return_if_fail(nfimage != NULL);
	g_return_if_fail(NFALIGN_LEFT <= align && NFALIGN_CENTER_RIGHT >= align);

	if(nfimage->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return;
	}

	nfimage->font_align = align;
	nfimage->font_margin = margin;
}

void nfui_nfimage_set_font_color(NFIMAGE *nfimage, guint font_color)
{
	g_return_if_fail(nfimage != NULL);

	if(nfimage->object.type != NFOBJECT_TYPE_NFIMAGE)
	{
		return;
	}

	nfimage->font_color = font_color;
}
