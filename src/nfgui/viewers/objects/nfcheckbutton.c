

#include "nfcheckbutton.h"

#include "../../support/color.h"
#include "../../support/nf_ui_color.h"
#include "../../support/util.h"
#include "../../support/event_loop.h"
#include "ix_mem.h"
#include "../../tools/nf_ui_tool.h"

#define CHECK_LINE_BORDER				2


static gboolean nfcheck_button_event_handler(NFCHECKBUTTON *check, GdkEvent *event, gpointer data);
static void nfcheck_button_draw_button(NFCHECKBUTTON *check);
static void nfcheck_draw_outlines(NFOBJECT *obj);
static void nfcheck_button_draw_text(NFCHECKBUTTON *check, GdkDrawable *drawable, GdkGC *gc, GdkPixbuf *pm, guint x, guint y);

static void _set_check_type_normal(NFCHECKBUTTON *check)
{
	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint size_w, size_h;	
    gint i;
    
	chk_inact_img[0] = nfui_get_image_from_file((IMG_CHECK_OFF_N), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_CHECK_OFF_O), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_CHECK_OFF_P), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_CHECK_OFF_D), NULL);	

	chk_act_img[0] = nfui_get_image_from_file((IMG_CHECK_ON_N), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_CHECK_ON_O), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_CHECK_ON_P), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_CHECK_ON_D), NULL);

	nfui_get_pixbuf_size(chk_inact_img[0], &size_w, &size_h);

	for (i = 0; i < NFCHECK_INACTIVE_STATES; i++)
		check->inactive_image[i] = chk_inact_img[i];

	for (i = 0; i < NFCHECK_ACTIVE_STATES; i++)
		check->active_image[i] = chk_act_img[i];

	nfui_nfobject_set_size(check, size_w, size_h);
	check->skin_type = NFCHECK_TYPE_NORMAL;
}

static void _set_check_type_small(NFCHECKBUTTON *check)
{
	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint size_w, size_h;	
    gint i;
    
	chk_inact_img[0] = nfui_get_image_from_file((IMG_CHECK_OFF_SMALL_N), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_CHECK_OFF_SMALL_O), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_CHECK_OFF_SMALL_P), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_CHECK_OFF_SMALL_D), NULL);	

	chk_act_img[0] = nfui_get_image_from_file((IMG_CHECK_ON_SMALL_N), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_CHECK_ON_SMALL_O), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_CHECK_ON_SMALL_P), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_CHECK_ON_SMALL_D), NULL);

	nfui_get_pixbuf_size(chk_inact_img[0], &size_w, &size_h);

	for (i = 0; i < NFCHECK_INACTIVE_STATES; i++)
		check->inactive_image[i] = chk_inact_img[i];

	for (i = 0; i < NFCHECK_ACTIVE_STATES; i++)
		check->active_image[i] = chk_act_img[i];

	nfui_nfobject_set_size(check, size_w, size_h);
	check->skin_type = NFCHECK_TYPE_SMALL;
}

static void _set_check_type_popup_normal(NFCHECKBUTTON *check)
{
	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint size_w, size_h;	
    gint i;
    
	chk_inact_img[0] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_N), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_O), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_P), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_D), NULL);	

	chk_act_img[0] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_N), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_O), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_P), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_D), NULL);

	nfui_get_pixbuf_size(chk_inact_img[0], &size_w, &size_h);

	for (i = 0; i < NFCHECK_INACTIVE_STATES; i++)
		check->inactive_image[i] = chk_inact_img[i];

	for (i = 0; i < NFCHECK_ACTIVE_STATES; i++)
		check->active_image[i] = chk_act_img[i];

	nfui_nfobject_set_size(check, size_w, size_h);
	check->skin_type = NFCHECK_TYPE_POPUP_NORMAL;
}

static void _set_check_type_popup_small(NFCHECKBUTTON *check)
{
	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint size_w, size_h;	
    gint i;
    
	chk_inact_img[0] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_SMALL_N), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_SMALL_O), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_SMALL_P), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_POPUP_CHECK_OFF_SMALL_D), NULL);	

	chk_act_img[0] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_SMALL_N), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_SMALL_O), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_SMALL_P), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_POPUP_CHECK_ON_SMALL_D), NULL);

	nfui_get_pixbuf_size(chk_inact_img[0], &size_w, &size_h);

	for (i = 0; i < NFCHECK_INACTIVE_STATES; i++)
		check->inactive_image[i] = chk_inact_img[i];

	for (i = 0; i < NFCHECK_ACTIVE_STATES; i++)
		check->active_image[i] = chk_act_img[i];

	nfui_nfobject_set_size(check, size_w, size_h);
	check->skin_type = NFCHECK_TYPE_POPUP_SMALL;
}

static void _set_check_type_subtab_normal(NFCHECKBUTTON *check)
{
	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint size_w, size_h;	
    gint i;
    
	chk_inact_img[0] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_N), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_O), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_P), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_D), NULL);	

	chk_act_img[0] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_N), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_O), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_P), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_D), NULL);

	nfui_get_pixbuf_size(chk_inact_img[0], &size_w, &size_h);

	for (i = 0; i < NFCHECK_INACTIVE_STATES; i++)
		check->inactive_image[i] = chk_inact_img[i];

	for (i = 0; i < NFCHECK_ACTIVE_STATES; i++)
		check->active_image[i] = chk_act_img[i];

	nfui_nfobject_set_size(check, size_w, size_h);
	check->skin_type = NFCHECK_TYPE_SUBTAB_NORMAL;
}

static void _set_check_type_subtab_small(NFCHECKBUTTON *check)
{
	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint size_w, size_h;	
    gint i;
    
	chk_inact_img[0] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_SMALL_N), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_SMALL_O), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_SMALL_P), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_OFF_SMALL_D), NULL);	

	chk_act_img[0] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_SMALL_N), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_SMALL_O), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_SMALL_P), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_SUBTAB_CHECK_ON_SMALL_D), NULL);

	nfui_get_pixbuf_size(chk_inact_img[0], &size_w, &size_h);

	for (i = 0; i < NFCHECK_INACTIVE_STATES; i++)
		check->inactive_image[i] = chk_inact_img[i];

	for (i = 0; i < NFCHECK_ACTIVE_STATES; i++)
		check->active_image[i] = chk_act_img[i];

	nfui_nfobject_set_size(check, size_w, size_h);
	check->skin_type = NFCHECK_TYPE_SUBTAB_SMALL;
}

static void _set_check_type_def(NFCHECKBUTTON *check)
{
	check->skin_type = NFCHECK_TYPE_DEF;
}

static gboolean nfcheck_button_event_handler(NFCHECKBUTTON *check, GdkEvent *event, gpointer data)
{
	g_assert(check->skin_type);

	switch(event->type)
	{	
		case GDK_EXPOSE:
        {
			if(!nfui_nfobject_is_disabled((NFOBJECT*)check)) 
			{
				if(check->object.kfocus == NFOBJECT_FOCUS) 
					check->object.status = NFOBJECT_STATE_PRELIGHT;
				else 
					check->object.status = NFOBJECT_STATE_NORMAL;
			}
        
			nfcheck_button_draw_button(check);
        }	
		break;

		case GDK_BUTTON_PRESS:
		{
			if(nfui_nfobject_is_disabled((NFOBJECT*)check))	break;
            if(event->button.button == MOUSE_RIGTH_BUTTON)  break;

			if(check->object.status != NFOBJECT_STATE_ACTIVE)
			{
				check->object.status = NFOBJECT_STATE_ACTIVE;
				nfcheck_button_draw_button(check);
			}
		}
		break;

		case GDK_BUTTON_RELEASE:
		{
			if(nfui_nfobject_is_disabled((NFOBJECT*)check))	break;
            if(event->button.button == MOUSE_RIGTH_BUTTON)  break;

			if(check->checked)  check->checked = 0;
			else				check->checked = 1;

            check->object.status = NFOBJECT_STATE_PRELIGHT;

			nfcheck_button_draw_button(check);			
			nfui_user_signal_emit((NFOBJECT*)check, NFEVENT_CHECKBUTTON_CHANGED, FALSE);
		}
		break;

		case GDK_ENTER_NOTIFY:
		{
			if(nfui_nfobject_is_disabled((NFOBJECT*)check))	break;
		
			check->object.status = NFOBJECT_STATE_PRELIGHT;
			nfcheck_button_draw_button(check);
		}
		break;
		
		case GDK_LEAVE_NOTIFY:
		{
			if(nfui_nfobject_is_disabled((NFOBJECT*)check))	break;

			if(check->object.kfocus == NFOBJECT_FOCUS)
				check->object.status = NFOBJECT_STATE_PRELIGHT;
			else
				check->object.status = NFOBJECT_STATE_NORMAL;

            nfcheck_button_draw_button(check);
		}
		break;
		
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			if(nfui_nfobject_is_disabled((NFOBJECT*)check))
				break;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_ENTER)
			{
				if(check->checked)
					check->checked = 0;
				else
					check->checked = 1;

				nfcheck_button_draw_button(check);
				nfui_user_signal_emit((NFOBJECT*)check, NFEVENT_CHECKBUTTON_CHANGED, FALSE);
				return TRUE;
			}
		}
		break;

		case GDK_DELETE:
			break;
		default:
			break;
	}

	return FALSE;
}


static void nfcheck_button_draw_button(NFCHECKBUTTON *check)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc = NULL;
	GdkPixbuf *pm = NULL;
	guint x, y;

	if(check->object.status < NFOBJECT_STATE_NORMAL 
		&& check->object.status > NFOBJECT_STATE_DISABLE) 
		return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)check);
	if(!drawable)	return;

	gc = nfui_nfobject_get_gc((NFOBJECT*)check);

	nfui_nfobject_get_offset((NFOBJECT*)check, &x, &y);

    if (check->checked)
        pm = check->active_image[check->object.status];
    else
        pm = check->inactive_image[check->object.status];
        
	nfutil_draw_pixbuf(drawable, gc, pm, x, y, -1, -1, NFALIGN_LEFT, 0);
	
	if(strlen(check->strLabel))
		nfcheck_button_draw_text(check, drawable, gc, pm, x, y);

	nfui_nfobject_gc_unref(gc);
}

static void nfcheck_button_draw_text(NFCHECKBUTTON *check, GdkDrawable *drawable, GdkGC *gc, GdkPixbuf *pm, guint x, guint y)
{
	gchar *key = NULL;
	gchar key_temp[NFCHECK_CASHING_KEY_SIZE];
	guint fg_color;
	guint w, h;

    if (check->checked)
        fg_color = check->active_font_color[check->object.status];
    else
        fg_color = check->inactive_font_color[check->object.status];

	if(check->valid_cnt < 0)
	{
		check->valid_cnt = nfutil_string_get_valid_count(1, drawable, check->pango_font, check->strLabel, check->object.width - check->font_margin);

#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
		if(check->valid_cnt == 0)
			nfui_nfobject_use_tooltip((NFOBJECT*)check, FALSE);
#endif
	}

	if(check->use_cashing)
	{
		if(check->cashing_key && strlen(check->cashing_key))
		{
			key = check->cashing_key;
		}
		else
		{
			memset(key_temp, 0, sizeof(key_temp));
			g_sprintf(key_temp, "PTR_IMG[%d]_%p", check->object.status, pm);

			key = key_temp;
		}
	}

	w = check->object.width;
	h = check->object.height;

#ifndef NF_TOOLTIP_ENABLE
	if(check->spacing_type == NORMAL_SPACING)
	{
		nfutil_draw_text_with_pango(key, pm, 
				NULL, drawable, gc, check->strLabel, 
				x, y, w, h, check->pango_font, 
				&UX_COLOR(fg_color), check->font_align, check->font_margin);
	}
	else
	{
		nfutil_draw_text_with_pango_spacing(key, pm, 
				NULL, drawable, gc, check->strLabel, 
				x, y, w, h, check->pango_font, 
				&UX_COLOR(fg_color), check->font_align, 
				check->font_margin, check->spacing_type);
	}
#else
	if(nftool_cur_language_is_eng())
	{
		nfutil_draw_short_text(key, pm, NULL, 
				drawable, gc, check->strLabel, 0, 
				x , y, w, h, check->pango_font,
				&UX_COLOR(fg_color), NULL, check->font_align, check->font_margin, 0, check->spacing_type);
	}
	else
	{
		nfutil_draw_short_text(key, pm, NULL, 
				drawable, gc, check->strLabel, check->valid_cnt,
				x, y, w, h, check->pango_font,
				&UX_COLOR(fg_color), NULL, check->font_align, check->font_margin, 0, check->spacing_type);
	}
#endif

}

static void nfcheck_draw_outlines(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(146));

	gdk_gc_set_line_attributes(line_gc,
			CHECK_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = CHECK_LINE_BORDER - 1;
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

NFOBJECT* nfui_checkbutton_new(gboolean active)
{
	NFCHECKBUTTON *check;

	check = (NFCHECKBUTTON*)imalloc(sizeof(NFCHECKBUTTON));
	if(check == NULL)	return NULL;

#if 0
	check->object.parent = NULL;
	check->object.x = 0;
	check->object.y = 0;
	check->object.width = 14;
	check->object.height = 14;
	check->object.type = NFOBJECT_TYPE_NFCHECKBUTTON;
	check->object.show = NFOBJECT_HIDE;
	check->object.use_focus = NFOBJECT_FOCUS_ON;
	check->object.kfocus = NFOBJECT_UNFOCUS;
	check->object.mfocus = NFOBJECT_UNFOCUS;
	check->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	check->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	check->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	check->object.pre_event_handler = NULL;
	check->object.default_event_handler = nfcheck_button_event_handler;
	check->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)check);

	check->object.width = 14;
	check->object.height = 14;
	check->object.type = NFOBJECT_TYPE_NFCHECKBUTTON;
	check->object.use_focus = NFOBJECT_FOCUS_ON;
	check->object.default_event_handler = nfcheck_button_event_handler;
#endif

	strcpy(check->font_name, "Calibri 16");
	check->pango_font = check->font_name;

#if 0
	if(active)
		check->status = NFOBJECT_STATE_ACTIVE;
	else
		check->status = NFOBJECT_STATE_NORMAL;
	check->sensitive = TRUE;
#else
	check->checked = active;
#endif
	check->use_cashing = 1;
	check->valid_cnt = -1;
	check->font_align = NFALIGN_CENTER;
	check->font_margin = 0;
	check->spacing_type = NORMAL_SPACING;
	check->skin_type = NFCHECK_TYPE_UNDEF;

	return (NFOBJECT*)check;
}

void nfui_check_button_set_skin_type(NFCHECKBUTTON *check, NFCHECK_TYPE type)
{
	g_return_val_if_fail(check != NULL, NULL);

	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
		return NULL;

	if (type == NFCHECK_TYPE_NORMAL)
		_set_check_type_normal(check);
	else if (type == NFCHECK_TYPE_SMALL)
		_set_check_type_small(check);
	else if (type == NFCHECK_TYPE_POPUP_NORMAL)
		_set_check_type_popup_normal(check);
	else if (type == NFCHECK_TYPE_POPUP_SMALL)
		_set_check_type_popup_small(check);	
	else if (type == NFCHECK_TYPE_SUBTAB_NORMAL)
		_set_check_type_subtab_normal(check);
	else if (type == NFCHECK_TYPE_SUBTAB_SMALL)
		_set_check_type_subtab_small(check);			
	else if (type == NFCHECK_TYPE_DEF)
		_set_check_type_def(check);	        		
}

void nfui_check_button_set_inactive_image(NFCHECKBUTTON *check, GdkPixbuf **img)
{
	gint i;

	g_return_if_fail(check != NULL); 
	g_return_if_fail(*img != NULL);
	
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

	for (i = 0; i < NFCHECK_INACTIVE_STATES; i++)
		check->inactive_image[i] = *(img+i);
}

void nfui_check_button_set_active_image(NFCHECKBUTTON *check, GdkPixbuf **img)
{
	gint i;

	g_return_if_fail(check != NULL); 
	g_return_if_fail(*img != NULL);
	
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

	for (i = 0; i < NFCHECK_ACTIVE_STATES; i++)
		check->active_image[i] = *(img+i);
}

gboolean nfui_check_button_get_active(NFCHECKBUTTON *check)
{
	g_return_val_if_fail(check != NULL, FALSE); 

	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return FALSE;
	}

	if(check->checked)
		return TRUE;
	else
		return FALSE;
}


void nfui_check_button_set_active(NFCHECKBUTTON *check, gboolean active)
{
	g_return_if_fail(check != NULL); 

	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

	if(active)	check->checked = 1;
	else		check->checked = 0;
		
	if(((NFOBJECT*)check)->parent && ((NFOBJECT*)check)->show) 
		nfcheck_button_draw_button(check);
}

void nfui_check_button_set_active_no_expose(NFCHECKBUTTON *check, gboolean active)
{
	g_return_if_fail(check != NULL); 

	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

	if(active)	check->checked = 1;
	else		check->checked = 0;
}

void nfui_check_button_sensitive(NFCHECKBUTTON *check, gboolean sensitive)
{
	g_return_if_fail(check != NULL); 
	
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

#if 0
	if(check->sensitive != sensitive)	
		check->sensitive = sensitive;
#else
	if(sensitive)	nfui_nfobject_enable((NFOBJECT*)check);
	else		nfui_nfobject_disable((NFOBJECT*)check);
#endif
}

void nfui_check_set_text(NFCHECKBUTTON *check, const gchar *strLabel)
{	
	gint length;

	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

	memset(check->strLabel, 0, sizeof(check->strLabel));
	length = strlen(strLabel);

	if(length > 0)	strncpy(check->strLabel, strLabel, sizeof(gchar)*length);

	if(check->object.use_tooltip)
	{
		nfui_nfobject_set_tooltip((NFOBJECT*)check, strLabel);
		check->valid_cnt = -1;
	}
}

gchar* nfui_check_get_text(NFCHECKBUTTON *check)
{	
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return NULL;
	}

	if(strlen(check->strLabel))
		return check->strLabel;
	else
		return NULL;
}

void nfui_check_set_inactive_pango_font(NFCHECKBUTTON *check, const gchar *pfont, guint *font_color)
{
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

//	strncpy(check->pango_font, pfont, sizeof(check->pango_font));

	if (pfont) {
		if (nffont_is_system_font(pfont))
			check->pango_font = pfont;
		else {
			strncpy(check->font_name, pfont, sizeof(check->font_name));
			check->pango_font = check->font_name;
		}
	}
	check->inactive_font_color[0] = font_color[0];
	check->inactive_font_color[1] = font_color[1];
	check->inactive_font_color[2] = font_color[2];
	check->inactive_font_color[3] = font_color[3];
}

void nfui_check_set_active_pango_font(NFCHECKBUTTON *check, const gchar *pfont, guint *font_color)
{
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

//	strncpy(check->pango_font, pfont, sizeof(check->pango_font));

	if (pfont) {
		if (nffont_is_system_font(pfont))
			check->pango_font = pfont;
		else {
			strncpy(check->font_name, pfont, sizeof(check->font_name));
			check->pango_font = check->font_name;
		}
	}
	check->active_font_color[0] = font_color[0];
	check->active_font_color[1] = font_color[1];
	check->active_font_color[2] = font_color[2];
	check->active_font_color[3] = font_color[3];
}

void nfui_check_use_pango_cashing(NFCHECKBUTTON *check, gint cashing, gchar *key)
{
	g_return_if_fail(check != NULL);
	
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

	if(cashing)	check->use_cashing = 1;
	else		check->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(check->cashing_key, key, sizeof(check->cashing_key));
	else
		memset(check->cashing_key, 0, sizeof(check->cashing_key));
}

void nfui_check_set_font_alignment(NFCHECKBUTTON *check, nfalign_type align, guint margin)
{
	g_return_if_fail(check != NULL); 
	g_return_if_fail(NFALIGN_LEFT <= align && NFALIGN_CENTER_RIGHT >= align);
	
	if(check->object.type != NFOBJECT_TYPE_NFCHECKBUTTON)
	{
		return;
	}

	check->font_align = align;
	check->font_margin = margin;
}

void nfui_check_get_size(NFCHECKBUTTON *check, gint *width, gint *height)
{
	g_assert(check->skin_type);
	g_assert(check->skin_type != NFCHECK_TYPE_DEF);

   	nfui_get_pixbuf_size(check->inactive_image[0], width, height); 
}

