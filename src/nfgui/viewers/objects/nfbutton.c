#include <string.h>

#include "nfbutton.h"

#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_color.h"

#include "ix_mem.h"

#include "tools/nf_ui_tool.h"

#include "iux_afx.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"NFBUTTON"


#define BUTTON_LINE_BORDER			2


static void nfbutton_draw_button(NFBUTTON *nfbutton);
static void nfbutton_draw_outlines(NFOBJECT *obj);



static gboolean nfbutton_event_handler(NFBUTTON *nfbutton, GdkEvent *event, gpointer data)
{
	g_assert(nfbutton);
	g_assert(event);

	switch(event->type) {	
		case GDK_EXPOSE:
// skshin		
			if(!nfbutton->toggled && !nfui_nfobject_is_disabled((NFOBJECT*)nfbutton)) 
			{
				if(nfbutton->object.kfocus == NFOBJECT_FOCUS) 
					nfbutton->object.status = NFOBJECT_STATE_PRELIGHT;
				else 
					nfbutton->object.status = NFOBJECT_STATE_NORMAL;
			}

			nfbutton_draw_button(nfbutton);
			break;

		case GDK_BUTTON_PRESS:
			if(!nfbutton->sensitive || nfui_nfobject_is_disabled((NFOBJECT*)nfbutton))
				break;

		      if(event->button.button == MOUSE_RIGTH_BUTTON)
		      {
		        break;
		      }
			  
			if(nfbutton->object.status != NFOBJECT_STATE_ACTIVE)
			{
				nfbutton->object.status = NFOBJECT_STATE_ACTIVE;
// skshin				
				nfbutton_draw_button(nfbutton);
//					nfui_signal_emit(nfbutton, GDK_EXPOSE, FALSE);
			}

			if(nfbutton->group->next)
			{
				GSList *tmp;

				tmp = nfbutton->group;

				while(tmp)
				{
					if(((NFBUTTON*)tmp->data)->toggled && (nfbutton != (NFBUTTON*)tmp->data))
					{
						((NFBUTTON*)tmp->data)->toggled = FALSE;
						((NFBUTTON*)tmp->data)->object.status = NFOBJECT_STATE_NORMAL;
						nfui_user_signal_emit((NFOBJECT*)(tmp->data), NFEVENT_RADIO_LOST_FOCUS, FALSE);
// skshin

						nfbutton_draw_button((NFBUTTON*)tmp->data);
//					nfui_signal_emit((NFBUTTON*)tmp->data, GDK_EXPOSE, FALSE);
						break;
					}
					tmp = tmp->next;
				}

				if(!nfbutton->toggled) 
				{
					nfbutton->toggled = TRUE;
					nfui_user_signal_emit((NFOBJECT*)nfbutton, NFEVENT_RADIO_GET_FOCUS, FALSE);
				}

// skshin
				nfbutton_draw_button(nfbutton);
//					nfui_signal_emit(nfbutton, GDK_EXPOSE, FALSE);

				nfui_user_signal_emit((NFOBJECT*)nfbutton, NFEVENT_RADIO_GET_FOCUS_ALWAYS, FALSE);
			}
			break;

		case GDK_BUTTON_RELEASE:
			if(!nfbutton->sensitive || nfui_nfobject_is_disabled((NFOBJECT*)nfbutton))
				break;

		      if(event->button.button == MOUSE_RIGTH_BUTTON)
		      {
		        break;
		      }


			if(!(nfbutton->group->next))
			{
				nfbutton->object.status = NFOBJECT_STATE_PRELIGHT;
// skshin				
				nfbutton_draw_button(nfbutton);
//					nfui_signal_emit(nfbutton, GDK_EXPOSE, FALSE);
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_PRESS:
		case NFEVENT_REMOCON_RELEASE:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)event;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_ENTER)
				{
					if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS) 
						nfui_left_button_signal_emit((NFOBJECT*)nfbutton, GDK_BUTTON_PRESS, FALSE);
					else if(event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_RELEASE) 
						nfui_left_button_signal_emit((NFOBJECT*)nfbutton, GDK_BUTTON_RELEASE, FALSE);

					return TRUE;
				}
			}
			break;

		case GDK_ENTER_NOTIFY:
			if(!nfbutton->sensitive || nfui_nfobject_is_disabled((NFOBJECT*)nfbutton))
				break;

			if(nfbutton->group->next)
			{
				if(nfbutton->object.status != NFOBJECT_STATE_ACTIVE)
				{
					nfbutton->object.status = NFOBJECT_STATE_PRELIGHT;
			// skshin					
					nfbutton_draw_button(nfbutton);
			//		nfui_signal_emit(nfbutton, GDK_EXPOSE, FALSE);
				}
			}
			else
			{
				nfbutton->object.status = NFOBJECT_STATE_PRELIGHT;
		//		nfui_signal_emit(nfbutton, GDK_EXPOSE, FALSE);
			// skshin					
				nfbutton_draw_button(nfbutton);
			}
			break;

		case GDK_LEAVE_NOTIFY:
			if(!nfbutton->sensitive || nfui_nfobject_is_disabled((NFOBJECT*)nfbutton))
				break;

			if(nfbutton->group->next)
			{
				if(nfbutton->object.status != NFOBJECT_STATE_ACTIVE)
				{
					nfbutton->object.status = NFOBJECT_STATE_NORMAL;
			//		nfui_signal_emit(nfbutton, GDK_EXPOSE, FALSE);
			// skshin					
					nfbutton_draw_button(nfbutton);
				}
			}
			else
			{
				if(nfbutton->object.kfocus == NFOBJECT_FOCUS)
					nfbutton->object.status = NFOBJECT_STATE_PRELIGHT;
				else
					nfbutton->object.status = NFOBJECT_STATE_NORMAL;

			// skshin					
				nfbutton_draw_button(nfbutton);
		//			nfui_signal_emit(nfbutton, GDK_EXPOSE, FALSE);
			}
			break;

		case GDK_DELETE:
			{
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static void nfbutton_draw_button(NFBUTTON *nfbutton)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint fg_color;
	gint bg_color = -1;
	guint x, y, w, h;
	guint icon_w = 0, icon_h = 0;
	gint icon_margin = 0;

	GdkPixbuf *pm = NULL;

	gchar *key = NULL;
	gchar key_temp[NFBUTTON_CASHING_KEY_SIZE];

	g_assert(nfbutton);

	if(!nfui_nfobject_is_shown((NFOBJECT*)nfbutton))
		return;

	if(nfbutton->object.status < NFOBJECT_STATE_NORMAL 
		&& nfbutton->object.status > NFOBJECT_STATE_DISABLE) 
		return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)nfbutton);
	gc = nfui_nfobject_get_gc((NFOBJECT*)nfbutton);

	nfui_nfobject_get_offset((NFOBJECT*)nfbutton, &x, &y);

	pm = nfbutton->image[nfbutton->object.status];

gdk_drawable_set_colormap(drawable, gdk_rgb_get_colormap());
	if(pm)
	{
g_assert(GDK_IS_PIXBUF(pm));
		nfui_get_pixbuf_size(pm, &w, &h);
		nfutil_draw_pixbuf(drawable, gc, pm, x, y, w, h, NFALIGN_LEFT, 0);
	}
	else
	{
		w = nfbutton->object.width;
		h = nfbutton->object.height;
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfui_nfobject_get_bg_color((NFOBJECT*)nfbutton, nfbutton->object.status)));
		gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
	}

	if(nfbutton->icon[nfbutton->object.status]) {
g_assert(GDK_IS_PIXBUF(nfbutton->icon[nfbutton->object.status]));
		nfui_get_pixbuf_size(nfbutton->icon[nfbutton->object.status], &icon_w, &icon_h);

		nfutil_draw_pixbuf(drawable, gc, nfbutton->icon[nfbutton->object.status], x + nfbutton->icon_pos.x, y + nfbutton->icon_pos.y, icon_w, icon_h, NFALIGN_LEFT, 0);

		icon_margin += (nfbutton->icon_pos.x + nfbutton->icon_pos.left_margin + nfbutton->icon_pos.right_margin + icon_w);
	}


	if(strlen(nfbutton->strLabel))
	{
		nfbutton->valid_cnt = nfutil_string_get_valid_count(1, drawable, nfbutton->pango_font, nfbutton->strLabel, nfbutton->object.width-nfbutton->font_margin-icon_margin);
#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
		if(nfbutton->valid_cnt == 0)
			nfui_nfobject_use_tooltip((NFOBJECT*)nfbutton, FALSE);
		else
			nfui_nfobject_use_tooltip((NFOBJECT*)nfbutton, TRUE);
#endif

		fg_color = nfbutton->font_color[nfbutton->object.status];
		bg_color = nfbutton->object.bg_color[nfbutton->object.status];

		if(nfbutton->use_cashing)
		{
			if(nfbutton->cashing_key && strlen(nfbutton->cashing_key))
			{
				key = nfbutton->cashing_key;
			}
			else
			{
				memset(key_temp, 0, sizeof(key_temp));

				if(nfbutton->image[nfbutton->object.status])
					g_sprintf(key_temp, "PTR_IMG[%d]_%p", nfbutton->object.status, nfbutton->image[nfbutton->object.status]);
				else
					g_sprintf(key_temp, "BGCOL_IDX[%d]_%d", nfbutton->object.status, bg_color);

				key = key_temp;
			}
		}
#ifndef NF_TOOLTIP_ENABLE
		if(nfbutton->spacing_type == NORMAL_SPACING)
		{
// skshin
//
			if (nfui_nfobject_is_supported_multi_lang(nfbutton)) {
				nfutil_draw_text_with_pango(key, NULL, 
					(bg_color<0 ? NULL : &UX_COLOR(bg_color)), 
						drawable, gc, nfbutton->strLabel, 
						x, y, w, h, nfbutton->pango_font, 
					&UX_COLOR(fg_color), nfbutton->font_align, nfbutton->font_margin);
			}
			else {
			nfutil_draw_text_with_pango_eng(key, NULL, 
						(bg_color<0 ? NULL : &colors[bg_color]), 
						drawable, gc, nfbutton->strLabel, 
						x, y, w, h, nfbutton->pango_font, 
						&colors[fg_color], nfbutton->font_align, nfbutton->font_margin);
			}
		}
		else
		{
			if (nfui_nfobject_is_supported_multi_lang(nfbutton)) {
				nfutil_draw_text_with_pango_spacing(key, NULL, 
					(bg_color<0 ? NULL : &UX_COLOR(bg_color)), 
						drawable, gc, nfbutton->strLabel, 
						x, y, w, h, nfbutton->pango_font, 
					&UX_COLOR(fg_color), nfbutton->font_align, 
						nfbutton->font_margin, nfbutton->spacing_type);
			}
			else {
				nfutil_draw_text_with_pango_spacing_eng(key, NULL, 
						(bg_color<0 ? NULL : &colors[bg_color]), 
						drawable, gc, nfbutton->strLabel, 
						x, y, w, h, nfbutton->pango_font, 
						&colors[fg_color], nfbutton->font_align, 
						nfbutton->font_margin, nfbutton->spacing_type);
			}
		}
#else
		if(nfbutton->icon[nfbutton->object.status]) {
			if(nfbutton->icon_pos.left_margin) {
				w -= icon_margin;
			} else if(nfbutton->icon_pos.right_margin) {
				x += icon_margin;
				w -= icon_margin;
			} else {
				x += icon_w; 
				w -= icon_w;
			}
		}

//		if(nftool_cur_language_is_eng())
		if(!nfui_nfobject_is_supported_multi_lang(nfbutton))
		{
// skshin			
			nfutil_draw_short_text_eng(key, NULL, (bg_color<0 ? NULL : &UX_COLOR(bg_color)), 
					drawable, gc, nfbutton->strLabel, 0, 
					//(x + icon_w), y, (w - icon_w), h, nfbutton->pango_font,
					x, y, w, h, nfbutton->pango_font,
					&UX_COLOR(fg_color), NULL, nfbutton->font_align, nfbutton->font_margin, 0, nfbutton->spacing_type);
/*			nfutil_draw_short_text(key, pm, (bg_color<0 ? NULL : &colors[bg_color]), 
					drawable, gc, nfbutton->strLabel, 0, 
					(x + icon_w), y, (w - icon_w), h, nfbutton->pango_font,
					&colors[fg_color], NULL, nfbutton->font_align, nfbutton->font_margin, 0, nfbutton->spacing_type);*/
		}
		else
		{
// skshin			
			nfutil_draw_short_text(key, NULL, (bg_color<0 ? NULL : &UX_COLOR(bg_color)), 
					drawable, gc, nfbutton->strLabel, nfbutton->valid_cnt,
					//(x + icon_w), y, (w - icon_w), h, nfbutton->pango_font,
					x, y, w, h, nfbutton->pango_font,
					&UX_COLOR(fg_color), NULL, nfbutton->font_align, nfbutton->font_margin, 0, nfbutton->spacing_type);
/*			nfutil_draw_short_text(key, pm, (bg_color<0 ? NULL : &colors[bg_color]), 
					drawable, gc, nfbutton->strLabel, nfbutton->valid_cnt,
					(x + icon_w), y, (w - icon_w), h, nfbutton->pango_font,
					&colors[fg_color], NULL, nfbutton->font_align, nfbutton->font_margin, 0, nfbutton->spacing_type);*/

		}
#endif
	}

	nfui_nfobject_gc_unref(gc);

	if(nfbutton->group->next) {
		if(nfbutton->draw_outline
					&& nfbutton->object.kfocus == NFOBJECT_FOCUS) 
			nfbutton_draw_outlines((NFOBJECT*)nfbutton);
	}
}

static void nfbutton_draw_outlines(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *line_gc;
	gint line_gap;
	guint x, y;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(146));

	gdk_gc_set_line_attributes(line_gc,
			BUTTON_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);
	
	nfui_nfobject_get_offset(obj, &x, &y);
	line_gap = BUTTON_LINE_BORDER - 1;
/*
	points[0].x = x + line_gap;
	points[0].y = y + line_gap;
	points[1].x = x + obj->width - line_gap;
	points[1].y = y + line_gap;
	points[2].x = x + obj->width - line_gap;
	points[2].y = y + obj->height - line_gap;
	points[3].x = x + line_gap;
	points[3].y = y + obj->height - line_gap;
	points[4].x = x + line_gap;
	points[4].y = y + line_gap;

	gdk_draw_lines(drawable,
			line_gc,
			points,
			5);
*/
	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					x + line_gap,
					y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);
}


NFBUTTON* nfui_nfbutton_new()
{
	NFBUTTON *nfbutton;

	nfbutton = (NFBUTTON*)imalloc(sizeof(NFBUTTON));
	if(nfbutton == NULL)	return NULL;

#if 0
	nfbutton->object.parent = NULL;
	nfbutton->object.x = 0;
	nfbutton->object.y = 0;
	nfbutton->object.width = 200;
	nfbutton->object.height = 100;
	nfbutton->object.type = NFOBJECT_TYPE_NFBUTTON;
	nfbutton->object.show = NFOBJECT_HIDE;
	nfbutton->object.use_focus = NFOBJECT_FOCUS_ON;
	nfbutton->object.kfocus = NFOBJECT_UNFOCUS;
	nfbutton->object.mfocus = NFOBJECT_UNFOCUS;
	nfbutton->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	nfbutton->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	nfbutton->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	nfbutton->object.bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;
	nfbutton->object.pre_event_handler = NULL;
	nfbutton->object.default_event_handler = nfbutton_event_handler;
	nfbutton->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)nfbutton);
	nfbutton->object.bg_color[NFOBJECT_STATE_DISABLE] = -1;

	nfbutton->object.width = 200;
	nfbutton->object.height = 100;
	nfbutton->object.type = NFOBJECT_TYPE_NFBUTTON;
	nfbutton->object.use_focus = NFOBJECT_FOCUS_ON;
	nfbutton->object.default_event_handler = nfbutton_event_handler;

	nfbutton->object.use_tooltip = 1;
#endif

	nfbutton->status = NFOBJECT_STATE_NORMAL;
	nfbutton->image[0] = NULL;
	nfbutton->image[1] = NULL;
	nfbutton->image[2] = NULL;
	nfbutton->image[3] = NULL;
	nfbutton->icon[0] = NULL;
	nfbutton->icon[1] = NULL;
	nfbutton->icon[2] = NULL;
	nfbutton->icon[3] = NULL;

	nfbutton->use_cashing = 1;
	nfbutton->valid_cnt = -1;
	nfbutton->font_align = NFALIGN_CENTER;
	nfbutton->font_margin = 0;

	strcpy(nfbutton->font_name, "Calibri 16");
	nfbutton->pango_font = nfbutton->font_name;
	
	
	nfbutton->sensitive = TRUE;

	nfbutton->draw_outline = TRUE;

	/* For Radio Button */
	nfbutton->toggled = FALSE;
	nfbutton->group = g_slist_prepend(NULL, nfbutton);	// FIXME glist_free??

	nfbutton->spacing_type = NORMAL_SPACING;

	memset(&nfbutton->icon_pos, 0x00, sizeof(ICONPOSITION));

	return nfbutton;
}

NFBUTTON* nfui_nfbutton_new_with_param(GdkPixbuf **img, const gchar *strLabel)
{
	NFBUTTON *nfbutton;
	guint i;
	gint length;

	nfbutton = nfui_nfbutton_new();
	if(nfbutton == NULL)	return NULL;

	if(img)
	{
		for(i=0; i<NFOBJECT_STATE_COUNT; i++)
			nfbutton->image[i] = *(img+i);

		g_assert(GDK_IS_PIXBUF(img[0]));
		nfui_get_pixbuf_size(img[0], &(nfbutton->object.width), &(nfbutton->object.height));
	}

	length = strlen(strLabel);

	if(length > 0 && nfbutton->object.use_tooltip)
	{
		strncpy(nfbutton->strLabel, strLabel, sizeof(gchar)*length);
		nfui_nfobject_set_tooltip((NFOBJECT*)nfbutton, strLabel);
	}

	return nfbutton;
}

NFBUTTON* nfui_nfbutton_new_no_image(gint *bg_color, const gchar *strLabel)
{	
	NFBUTTON *nfbutton;
	guint i;
	gint length;

	nfbutton = nfui_nfbutton_new();
	if(nfbutton == NULL)	return NULL;

	nfbutton->object.bg_color[0] = bg_color[0];
	nfbutton->object.bg_color[1] = bg_color[1];
	nfbutton->object.bg_color[2] = bg_color[2];
	nfbutton->object.bg_color[3] = bg_color[3];

	length = strlen(strLabel);

	if(length > 0 && nfbutton->object.use_tooltip)
	{
		strncpy(nfbutton->strLabel, strLabel, sizeof(gchar)*length);
		nfui_nfobject_set_tooltip((NFOBJECT*)nfbutton, strLabel);
	}

	return nfbutton;
}

void nfui_nfbutton_destroy(NFBUTTON *button)
{
}

void nfui_nfbutton_set_image(NFBUTTON *button, GdkPixbuf **img)
{
	gint i;

	g_return_if_fail(button != NULL); 
	g_return_if_fail(*img != NULL);

	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}
	
	for(i=0; i<NFOBJECT_STATE_COUNT; i++)
	{
		if(!(*(img+i)))
		{
			DMSG(1, "NFBUTTON IMAGE ERROR");
		}

		button->image[i] = *(img+i);
	}

g_assert(GDK_IS_PIXBUF(img[0]));
	nfui_get_pixbuf_size(img[0], &(button->object.width), &(button->object.height));
}

void nfui_nfbutton_set_icon_image(NFBUTTON *button, GdkPixbuf **icon)
{
	gint i;

	g_return_if_fail(button != NULL); 
	g_return_if_fail(icon != NULL);

	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}
	
	for(i=0; i<NFOBJECT_STATE_COUNT; i++)
	{
		if(!(*(icon+i)))
		{
			DMSG(1, "NFBUTTON IMAGE ERROR");
		}

		button->icon[i] = *(icon+i);
	}
}


void nfui_nfbutton_set_text(NFBUTTON *button, const gchar *strLabel)
{	
	gint length;

	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	memset(button->strLabel, 0, sizeof(button->strLabel));
	length = strlen(strLabel);

	if(length > 0)	strncpy(button->strLabel, strLabel, sizeof(gchar)*length);

	if(button->object.use_tooltip)
	{
		nfui_nfobject_set_tooltip((NFOBJECT*)button, strLabel);
		button->valid_cnt = -1;
	}
}

gchar* nfui_nfbutton_get_text(NFBUTTON *button)
{	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return NULL;
	}

	// SKSHIN
	#if 0
	if(strlen(button->strLabel))
		return button->strLabel;
	else
		return NULL;
	#endif

	// SKSHIN add below		
	return button->strLabel;
}

void nfui_nfbutton_set_pango_font(NFBUTTON *button, const gchar *pfont, guint *font_color)
{
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

//	strncpy(button->pango_font, pfont, sizeof(button->pango_font));

	if (pfont) {
		if (nffont_is_system_font(pfont))
			button->pango_font = pfont;
		else {
			strncpy(button->font_name, pfont, sizeof(button->font_name));
			button->pango_font = button->font_name;
		}
	}

	if (font_color) {
	button->font_color[0] = font_color[0];
	button->font_color[1] = font_color[1];
	button->font_color[2] = font_color[2];
	button->font_color[3] = font_color[3];
}
}

void nfui_nfbutton_use_pango_cashing(NFBUTTON *button, gint cashing, gchar *key)
{
	g_return_if_fail(button != NULL);
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	if(cashing)	button->use_cashing = 1;
	else		button->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(button->cashing_key, key, sizeof(button->cashing_key));
	else
		memset(button->cashing_key, 0, sizeof(button->cashing_key));
}

void nfui_nfbutton_set_bg_color(NFBUTTON *button, gint *bg_color)
{
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	button->object.bg_color[0] = bg_color[0];
	button->object.bg_color[1] = bg_color[1];
	button->object.bg_color[2] = bg_color[2];
	button->object.bg_color[3] = bg_color[3];
}

/* For Radio Button */
GSList* nfui_radio_button_get_group(NFBUTTON *button)
{
	g_return_val_if_fail(button != NULL, NULL); 

	return button->group;
}


void nfui_radio_button_add_group(NFBUTTON *button, GSList *group)
{
	g_return_if_fail(button != NULL); 
	g_return_if_fail(group != NULL); 

	button->group = g_slist_append(group, button);
}

void nfui_radio_button_set_toggled(NFBUTTON *button, gboolean toggled)
{
	GSList *tmp = NULL;

	g_return_if_fail(button != NULL); 

	tmp = button->group;
	while(tmp) 
	{
		if(((NFBUTTON*)tmp->data)->toggled == toggled && (button != (NFBUTTON*)(tmp->data))) 
		{
			((NFBUTTON*)tmp->data)->toggled = !toggled;
			if (((NFBUTTON*)tmp->data)->object.status != NFOBJECT_STATE_DISABLE) {
				((NFBUTTON*)tmp->data)->object.status = NFOBJECT_STATE_NORMAL;
			}
			break;
		}
		tmp = tmp->next;
	}

	button->toggled = toggled;
	button->object.status = NFOBJECT_STATE_ACTIVE;
}

void nfui_radio_button_unset_toggled(NFBUTTON *button)
{
	g_return_if_fail(button != NULL); 

	button->toggled = FALSE;
	button->object.status = NFOBJECT_STATE_NORMAL;
}

gboolean nfui_radio_button_get_toggled(NFBUTTON *button)
{
	g_return_val_if_fail(button != NULL, FALSE); 
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return FALSE;
	}

	return button->toggled;
}

gint nfui_radio_button_get_index(NFBUTTON *button)
{
	gint ret = -1;

	g_return_val_if_fail(button != NULL, FALSE); 
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return FALSE;
	}

	if(button->group) 
		ret = g_slist_index(button->group, button);

	return ret;
}

void nfui_nfbutton_sensitive(NFBUTTON *button, gboolean sensitive)
{
	g_return_if_fail(button != NULL); 
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	if(button->sensitive != sensitive)	
		button->sensitive = sensitive;
}

void nfui_nfbutton_set_font_alignment(NFBUTTON *button, nfalign_type align, guint margin)
{
	g_return_if_fail(button != NULL); 
	g_return_if_fail(NFALIGN_LEFT <= align && NFALIGN_CENTER_DOWN >= align);
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	button->font_align = align;
	button->font_margin = margin;
}

void nfui_nfbutton_set_drawing_outline(NFBUTTON *button, gboolean draw_outline)
{
	g_return_if_fail(button != NULL);
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	if(button->draw_outline != draw_outline)
		button->draw_outline = draw_outline;
}


void nfui_nfbutton_set_spacing(NFBUTTON *button, nfutil_pango_spacing_type spacing_type)
{
	g_return_if_fail(button != NULL);
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	button->spacing_type = spacing_type;
}




void nfui_nfbutton_set_status(NFBUTTON *button, nfobject_state status)
{
	g_return_if_fail(button != NULL);
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}

	button->object.status = status;
// skshin	
	nfbutton_draw_button(button);
//					nfui_signal_emit(button, GDK_EXPOSE, FALSE);
}

void nfui_nfbutton_set_icon_position(NFBUTTON *button, ICONPOSITION position)
{
	g_return_if_fail(button != NULL);
	
	if(button->object.type != NFOBJECT_TYPE_NFBUTTON)
	{
		return;
	}
	
	memcpy(&button->icon_pos, &position, sizeof(ICONPOSITION));
}
