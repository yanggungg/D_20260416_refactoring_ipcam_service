#include <math.h>
#include "../../support/nf_ui_font.h"
#include "../../support/event_loop.h"
#include "../../support/color.h"


#include "nflabel.h"
#include "nfbutton.h"
#include "nfspinbutton.h"
#include "ix_mem.h"

#include "../../tools/nf_ui_tool.h"

#define	SPINKEY_REPEAT_INTERVAL		(300)

#define SPIN_LINE_BORDER				2


typedef enum {
	GDOUBLE_TO_F_CHAR,
	GDOUBLE_TO_E_CHAR,
	GDOUBLE_TO_G_CHAR
}ConvType;


static void prvUpButtonProc(NFSPINBUTTON *spin, gboolean event_send);
static void prvDownButtonProc(NFSPINBUTTON *spin, gboolean event_send);


static gboolean nfspinbutton_event_handler(NFSPINBUTTON *spin, GdkEvent *event, gpointer data);
static void nfspinbutton_set_bg_gc(NFSPINBUTTON *spin);
static void nfspinbutton_set_child_position(NFSPINBUTTON *spin);
static void nfspinbutton_draw_bg(NFSPINBUTTON *spin);
static gboolean nfspinbutton_label_cb(NFOBJECT *label, GdkEvent *event, gpointer data);
static gboolean nfspinbutton_press_cb(NFOBJECT *button, GdkEvent *event, gpointer data);
static void nfspinbutton_text_rendering(NFSPINBUTTON *spin, gchar* str);
static gchar* nfspin_button_convert_gdouble_to_char(NFSPINBUTTON *spin, ConvType type, gdouble value);
static void nfspin_draw_outlines(NFOBJECT *obj);
static void nfspinbutton_change_child_position(NFSPINBUTTON *spin);



static void  _set_spinbutton_type_1(NFSPINBUTTON *spin)
{
	GdkPixbuf *spin_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *spin_down[NFOBJECT_STATE_COUNT];
	gint width, height;
	gint color[NFOBJECT_STATE_COUNT];	

	spin_up[0] = nfui_get_image_from_file((IMG_N_SPIN_UP), NULL);
	spin_up[1] = nfui_get_image_from_file((IMG_O_SPIN_UP), NULL);	
	spin_up[2] = nfui_get_image_from_file((IMG_P_SPIN_UP), NULL);	
	spin_up[3] = nfui_get_image_from_file((IMG_D_SPIN_UP), NULL);	

	spin_down[0] = nfui_get_image_from_file((IMG_N_SPIN_DOWN), NULL);
	spin_down[1] = nfui_get_image_from_file((IMG_O_SPIN_DOWN), NULL);
	spin_down[2] = nfui_get_image_from_file((IMG_P_SPIN_DOWN), NULL);
	spin_down[3] = nfui_get_image_from_file((IMG_D_SPIN_DOWN), NULL);

	nfui_get_pixbuf_size(spin_up[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->up_btn), spin_up);
	nfui_nfobject_set_size(spin->up_btn, width, height);

	nfui_get_pixbuf_size(spin_down[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->down_btn), spin_down);
	nfui_nfobject_set_size(spin->down_btn, width, height);

	nfui_nflabel_set_pango_font((NFLABEL*)(spin->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(177));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(177);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(179);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(181);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(183);
    nfui_nflabel_set_fg_color((NFOBJECT*)spin->label, color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)spin->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(182));		

	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_NORMAL, COLOR_IDX(176));	
	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_DISABLE, COLOR_IDX(182));	

	spin->skin_type = NFSPINBUTTON_TYPE_1;
}

static void  _set_spinbutton_type_popup_1(NFSPINBUTTON *spin)
{
	GdkPixbuf *spin_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *spin_down[NFOBJECT_STATE_COUNT];
	gint width, height;
	gint color[NFOBJECT_STATE_COUNT];	

	spin_up[0] = nfui_get_image_from_file((IMG_N_POPUP_SPIN_UP), NULL);
	spin_up[1] = nfui_get_image_from_file((IMG_O_POPUP_SPIN_UP), NULL);	
	spin_up[2] = nfui_get_image_from_file((IMG_P_POPUP_SPIN_UP), NULL);	
	spin_up[3] = nfui_get_image_from_file((IMG_D_POPUP_SPIN_UP), NULL);	

	spin_down[0] = nfui_get_image_from_file((IMG_N_POPUP_SPIN_DOWN), NULL);
	spin_down[1] = nfui_get_image_from_file((IMG_O_POPUP_SPIN_DOWN), NULL);
	spin_down[2] = nfui_get_image_from_file((IMG_P_POPUP_SPIN_DOWN), NULL);
	spin_down[3] = nfui_get_image_from_file((IMG_D_POPUP_SPIN_DOWN), NULL);

	nfui_get_pixbuf_size(spin_up[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->up_btn), spin_up);
	nfui_nfobject_set_size(spin->up_btn, width, height);

	nfui_get_pixbuf_size(spin_down[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->down_btn), spin_down);
	nfui_nfobject_set_size(spin->down_btn, width, height);

	nfui_nflabel_set_pango_font((NFLABEL*)(spin->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(269));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(269);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(271);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(273);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(275);
    nfui_nflabel_set_fg_color((NFOBJECT*)spin->label, color);	
	nfui_nfobject_modify_bg((NFOBJECT*)spin->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(274));		

	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_NORMAL, COLOR_IDX(268));	
	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_DISABLE, COLOR_IDX(274));	

	spin->skin_type = NFSPINBUTTON_TYPE_POPUP_1;
}

static void  _set_spinbutton_type_popup_small_1(NFSPINBUTTON *spin)
{
	GdkPixbuf *spin_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *spin_down[NFOBJECT_STATE_COUNT];
	gint width, height;
	gint color[NFOBJECT_STATE_COUNT];	

	spin_up[0] = nfui_get_image_from_file((MK_IMG_N_POPUP_SPIN_UP_SMALL), NULL);
	spin_up[1] = nfui_get_image_from_file((MK_IMG_O_POPUP_SPIN_UP_SMALL), NULL);	
	spin_up[2] = nfui_get_image_from_file((MK_IMG_P_POPUP_SPIN_UP_SMALL), NULL);	
	spin_up[3] = nfui_get_image_from_file((MK_IMG_D_POPUP_SPIN_UP_SMALL), NULL);	

	spin_down[0] = nfui_get_image_from_file((MK_IMG_N_POPUP_SPIN_DOWN_SMALL), NULL);
	spin_down[1] = nfui_get_image_from_file((MK_IMG_O_POPUP_SPIN_DOWN_SMALL), NULL);
	spin_down[2] = nfui_get_image_from_file((MK_IMG_P_POPUP_SPIN_DOWN_SMALL), NULL);
	spin_down[3] = nfui_get_image_from_file((MK_IMG_D_POPUP_SPIN_DOWN_SMALL), NULL);

	nfui_get_pixbuf_size(spin_up[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->up_btn), spin_up);
	nfui_nfobject_set_size(spin->up_btn, width, height);

	nfui_get_pixbuf_size(spin_down[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->down_btn), spin_down);
	nfui_nfobject_set_size(spin->down_btn, width, height);

	nfui_nflabel_set_pango_font((NFLABEL*)(spin->label), nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(269));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(269);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(271);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(273);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(275);
    nfui_nflabel_set_fg_color((NFOBJECT*)spin->label, color);	
	nfui_nfobject_modify_bg((NFOBJECT*)spin->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(274));		

	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_NORMAL, COLOR_IDX(268));	
	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_DISABLE, COLOR_IDX(274));	

	spin->skin_type = NFSPINBUTTON_TYPE_POPUP_SMALL_1;
}

static void  _set_spinbutton_type_subtab_1(NFSPINBUTTON *spin)
{
	GdkPixbuf *spin_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *spin_down[NFOBJECT_STATE_COUNT];
	gint width, height;
	gint color[NFOBJECT_STATE_COUNT];	

	spin_up[0] = nfui_get_image_from_file((IMG_N_SUBTAB_SPIN_UP), NULL);
	spin_up[1] = nfui_get_image_from_file((IMG_O_SUBTAB_SPIN_UP), NULL);	
	spin_up[2] = nfui_get_image_from_file((IMG_P_SUBTAB_SPIN_UP), NULL);	
	spin_up[3] = nfui_get_image_from_file((IMG_D_SUBTAB_SPIN_UP), NULL);	

	spin_down[0] = nfui_get_image_from_file((IMG_N_SUBTAB_SPIN_DOWN), NULL);
	spin_down[1] = nfui_get_image_from_file((IMG_O_SUBTAB_SPIN_DOWN), NULL);
	spin_down[2] = nfui_get_image_from_file((IMG_P_SUBTAB_SPIN_DOWN), NULL);
	spin_down[3] = nfui_get_image_from_file((IMG_D_SUBTAB_SPIN_DOWN), NULL);

	nfui_get_pixbuf_size(spin_up[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->up_btn), spin_up);
	nfui_nfobject_set_size(spin->up_btn, width, height);

	nfui_get_pixbuf_size(spin_down[0], &width, &height);
	nfui_nfbutton_set_image(NF_BUTTON(spin->down_btn), spin_down);
	nfui_nfobject_set_size(spin->down_btn, width, height);

	nfui_nflabel_set_pango_font((NFLABEL*)(spin->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(959));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(959);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(961);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(963);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(965);
    nfui_nflabel_set_fg_color((NFOBJECT*)spin->label, color);	
	nfui_nfobject_modify_bg((NFOBJECT*)spin->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(964));		

	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_NORMAL, COLOR_IDX(958));	
	nfui_nfobject_modify_bg((NFOBJECT*)spin, NFOBJECT_STATE_DISABLE, COLOR_IDX(964));	

	spin->skin_type = NFSPINBUTTON_TYPE_SUBTAB_1;
}

static gboolean repeat_key_proc_short(gpointer data)
{
	NFSPINBUTTON *spin;
       gboolean event_send = FALSE;
	spin = (NFSPINBUTTON*)data;

	if(!spin->krepeat_src && !spin->mrepeat_src)
		return FALSE;

	if(((NFOBJECT*)spin)->kfocus == NFOBJECT_UNFOCUS)
	{
		spin->krepeat_src = 0;
		spin->mrepeat_src = 0;

		return FALSE;
	}


	NFUTIL_THREADS_ENTER();
       if(spin->krepeat_src && spin->only_enterkey_action)
        {
            event_send = FALSE;
        }
       else
        {
            event_send = TRUE;
        }
       
	if(spin->rkey_id == KEYPAD_DOWN)		prvDownButtonProc(spin, event_send);
	else if(spin->rkey_id == KEYPAD_UP)		prvUpButtonProc(spin, event_send);
	else
	{
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}
	NFUTIL_THREADS_LEAVE();

	return TRUE;
}

static gboolean repeat_key_proc(gpointer data)
{
	NFSPINBUTTON *spin;
       gboolean event_send = FALSE;

	spin = (NFSPINBUTTON*)data;

	if(!spin->krepeat_src && !spin->mrepeat_src)
		return FALSE;

	if(((NFOBJECT*)spin)->kfocus == NFOBJECT_UNFOCUS)
	{
		spin->krepeat_src = 0;
		spin->mrepeat_src = 0;

		return FALSE;
	}


	NFUTIL_THREADS_ENTER();

       if(spin->krepeat_src && spin->only_enterkey_action)
        {
            event_send = FALSE;
        }
       else
        {
            event_send = TRUE;
        }

	if(spin->rkey_id == KEYPAD_DOWN)		prvDownButtonProc(spin, event_send);
	else if(spin->rkey_id == KEYPAD_UP)		prvUpButtonProc(spin, event_send);
	else
	{
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}
	NFUTIL_THREADS_LEAVE();

	if(spin->mrepeat_src)
		spin->mrepeat_src = g_timeout_add(SPINKEY_REPEAT_INTERVAL, repeat_key_proc_short, spin);
	else if(spin->krepeat_src)
		spin->krepeat_src = g_timeout_add(SPINKEY_REPEAT_INTERVAL, repeat_key_proc_short, spin);
	return FALSE;
}

static void nfspinbutton_set_bg_gc(NFSPINBUTTON *spin)
{
	GdkDrawable *drawable = NULL;
	GdkRectangle rect;

	g_return_if_fail(spin != NULL);

	drawable = nfui_nfobject_get_window((NFOBJECT*)spin);

	if(drawable) {
		spin->bg_gc = gdk_gc_new(drawable);
		nfui_nfobject_get_clip_rect(spin, &rect);
		gdk_gc_set_clip_rectangle(spin->bg_gc, &rect);
		gdk_gc_set_rgb_fg_color(spin->bg_gc, &UX_COLOR(spin->spin_fixed.object.bg_color[NFOBJECT_STATE_NORMAL]));
	}else
		g_warning("%s : spin button drawable is NULL.", __FUNCTION__);
}


static void nfspinbutton_set_child_position(NFSPINBUTTON *spin)
{
	guint w, h;

	g_return_if_fail(spin != NULL);

	w = spin->spin_fixed.object.width;
	h = spin->spin_fixed.object.height;

	nfui_nfobject_set_size(spin->label, w-spin->up_btn->width-4, h-4);
	nfui_nffixed_put((NFFIXED*)spin, spin->label, 2, 2);

	nfui_nffixed_put((NFFIXED*)spin, 
					spin->up_btn, 
					w - spin->up_btn->width, 
//					1);
					0);

	nfui_nffixed_put((NFFIXED*)spin, 
					spin->down_btn, 
					w - spin->down_btn->width, 
//					spin->down_btn->height+1);
					spin->down_btn->height);
}

static void nfspinbutton_change_child_position(NFSPINBUTTON *spin)
{
	guint w, h;

	g_return_if_fail(spin != NULL);

	w = spin->spin_fixed.object.width;
	h = spin->spin_fixed.object.height;

	nfui_nfobject_set_size(spin->label, w-spin->up_btn->width-4, h-4);

	((NFOBJECT *)spin->label)->x = 2;
	((NFOBJECT *)spin->label)->y = 2;

	((NFOBJECT *)spin->up_btn)->x = w - spin->up_btn->width;
	((NFOBJECT *)spin->up_btn)->y = 0;

	((NFOBJECT *)spin->down_btn)->x = w - spin->down_btn->width;
	((NFOBJECT *)spin->down_btn)->y = spin->down_btn->height;

}


static void nfspinbutton_draw_bg(NFSPINBUTTON *spin)
{
	GdkDrawable *drawable = NULL;
	guint px, py;
	GdkRectangle rect;

	g_return_if_fail(spin != NULL);

	if(nfui_nfobject_is_disabled((NFOBJECT*)spin))
		return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)spin);
	nfui_nfobject_get_offset((NFOBJECT*)spin, &px, &py);

	nfui_nfobject_get_clip_rect((NFOBJECT*)spin, &rect);
	gdk_gc_set_clip_rectangle(spin->bg_gc, &rect);	

	if(drawable)
	{
		gdk_draw_rectangle(drawable,
			spin->bg_gc,
			TRUE,
			(gint)px,
			(gint)py,
			(gint)(spin->spin_fixed.object.width - spin->up_btn->width),
			(gint)spin->spin_fixed.object.height);
	}
	else
	{
		g_warning("%s : spin button drawable is NULL.", __FUNCTION__);

		return;
	}

	if(spin->spin_fixed.object.kfocus == NFOBJECT_FOCUS && spin->draw_outline) 
		nfspin_draw_outlines((NFOBJECT*)spin);
}

static void nfspin_draw_outlines(NFOBJECT *obj)
{
	NFSPINBUTTON *spin;
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;
	gint line_width;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);

	if(nfui_is_focus_at_child(obj))
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(185));
	else
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(184));

	gdk_gc_set_line_attributes(line_gc,
			SPIN_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = SPIN_LINE_BORDER - 1;
	line_width = obj->width - NF_SPINBUTTON(obj)->up_btn->width;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					line_width - (line_gap * 2),
					obj->height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);
}

static void prvUpButtonProc(NFSPINBUTTON *spin, gboolean event_send)
{
	gchar* str = NULL;
	guint8 src_n = 0;
	gdouble max_val;
    gdouble new_value = 0.0;

	if (spin->adjustment) 
	{
        new_value = spin->adjustment->value + spin->adjustment->step_increment;

        if (fabs(spin->adjustment->value - spin->adjustment->upper) < 1e-10)
        {
            new_value = spin->adjustment->lower;
        }
        else if (new_value > spin->adjustment->upper)
        {
            new_value = spin->adjustment->upper;

			if(spin->krepeat_src) 		src_n = 1;
			else if(spin->mrepeat_src)	src_n = 2;            
        }

        spin->adjustment->value = new_value;
        spin->index = round((spin->adjustment->value - spin->adjustment->lower)/spin->adjustment->step_increment);
        
		str = nfspin_button_convert_gdouble_to_char(spin, GDOUBLE_TO_G_CHAR, spin->adjustment->value);
		nfspinbutton_draw_bg(spin);

		if (nfui_nfobject_is_supported_multi_lang(spin))
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
		else
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

		nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
		
		g_free(str);
    }
	else 
	{
		str = g_slist_nth_data(spin->data, (guint)(++spin->index));
		
		if (!str) 
		{
			if (spin->krepeat_src || spin->mrepeat_src) 
			{
				--spin->index;
				str = g_slist_nth_data(spin->data, (guint)spin->index);

				if(spin->krepeat_src) 		src_n = 1;
				else if(spin->mrepeat_src)	src_n = 2;
			}
			else 
			{
				spin->index -= spin->index;
				str = g_slist_nth_data(spin->data, (guint)spin->index);
			}
		}

		nfspinbutton_draw_bg(spin);

		if (nfui_nfobject_is_supported_multi_lang(spin))
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
		else
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

		nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
	}

	if (src_n == 1) 
	{
		spin->krepeat_src = 0;
		spin->rkey_id = 0;
	}
	else if (src_n == 2) 
	{
		spin->mrepeat_src = 0;
		spin->rkey_id = 0;
	}

	nfui_signal_emit(spin->label, GDK_EXPOSE, TRUE);

    if (event_send == FALSE) return;
      
#ifdef	__RELEASE_EVENT_ENABLE__
	nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED_PRESS, FALSE);
#else
	nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
#endif
}

static void prvDownButtonProc(NFSPINBUTTON *spin, gboolean event_send)
{
	gchar* str = NULL;
	gdouble max_val;
    gdouble new_value = 0.0;	
	guint8 src_n = 0;
	gint i;

	if (spin->adjustment) 
	{
        new_value = spin->adjustment->value - spin->adjustment->step_increment;

        if (fabs(spin->adjustment->value - spin->adjustment->lower) < 1e-10)
        {
            new_value = spin->adjustment->upper;
        }
        else if (new_value < spin->adjustment->lower)
        {
            new_value = spin->adjustment->lower;

			if(spin->krepeat_src) 		src_n = 1;
			else if(spin->mrepeat_src)	src_n = 2;            
        }

        spin->adjustment->value = new_value;
        spin->index = round((spin->adjustment->value - spin->adjustment->lower)/spin->adjustment->step_increment);

		str = nfspin_button_convert_gdouble_to_char(spin, GDOUBLE_TO_G_CHAR, spin->adjustment->value);
		nfspinbutton_draw_bg(spin);

		if (nfui_nfobject_is_supported_multi_lang(spin))
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
		else
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

		nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
		
		g_free(str);
	}else {
		str = g_slist_nth_data(spin->data, (guint)(--spin->index));
		
		if (!str) 
		{
			if (spin->krepeat_src || spin->mrepeat_src) 
			{
				++spin->index;
				str = g_slist_nth_data(spin->data, (guint)(spin->index));

				if (spin->krepeat_src) 		src_n = 1;
				else if (spin->mrepeat_src)	src_n = 2;
			}
			else 
			{
				spin->index = g_slist_length(spin->data);
				spin->index -= 1;
				str = g_slist_nth_data(spin->data, (guint)spin->index);
			}
		}

		nfspinbutton_draw_bg(spin);

		if (nfui_nfobject_is_supported_multi_lang(spin))
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
		else
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

		nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
	}

	if (src_n == 1) 
	{
		spin->krepeat_src = 0;
		spin->rkey_id = 0;
	}
	else if (src_n == 2) 
	{
		spin->mrepeat_src = 0;
		spin->rkey_id = 0;
	}

	nfui_signal_emit(spin->label, GDK_EXPOSE, TRUE);

    if (event_send == FALSE) return;
	
#ifdef	__RELEASE_EVENT_ENABLE__.
	nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED_PRESS, FALSE);
#else
	nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
#endif
}

static gchar* nfspin_button_convert_gdouble_to_char(NFSPINBUTTON *spin, ConvType type, gdouble value)
{
	gchar *buf;

    buf = g_strdup_printf("%0.*f", spin->digits, value);
	return buf;
}


#if 0
static void nfspinbutton_text_rendering(NFSPINBUTTON *spin, gchar* str)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkColor *text_color = NULL;
	guint x, y, w, h;

	g_return_if_fail(str);

	drawable = nfui_nfobject_get_window((NFOBJECT*)spin);
	gc = nfui_nfobject_get_gc(drawable);
	text_color = gdk_color_copy(&colors[spin->font_color]);

	nfui_nfobject_get_offset((NFOBJECT*)spin, &x, &y);
	w = spin->spin_fixed.object.width - spin->up_btn->width;
	h = spin->spin_fixed.object.height;

	nfutil_draw_text(gc, drawable, str, x, y, w, h, spin->font, text_color, spin->align, spin->margin); 

	nfui_nfobject_gc_unref(gc);
	gdk_color_free(text_color);
}
#endif

static gboolean nfspinbutton_event_handler(NFSPINBUTTON *spin, GdkEvent *event, gpointer data)
{
	gchar *str;

	g_return_val_if_fail(spin != NULL, FALSE);
	g_assert(spin->skin_type);

	switch(event->type) {
		case GDK_EXPOSE:
			if(!spin->bg_gc) {
				nfspinbutton_set_bg_gc(spin);
			}

			if(spin->bg_gc) 
				nfspinbutton_draw_bg(spin);

			if(spin->spacing_type != NORMAL_SPACING)
			{
				nfui_nflabel_set_spacing((NFLABEL*)(spin->label), spin->spacing_type);
			}

			if(spin->adjustment)
			{
				str = nfspin_button_convert_gdouble_to_char(spin, GDOUBLE_TO_G_CHAR, spin->adjustment->value);

				if (nfui_nfobject_is_supported_multi_lang(spin))
					nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
				else
					nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

				nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
				g_free(str);

			}
			else
			{
				str = (gchar*)g_slist_nth_data(spin->data, (guint)spin->index);

				if (nfui_nfobject_is_supported_multi_lang(spin))
					nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
				else
					nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

				nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
			}

			// SKSHIN
			//if(!((NFFIXED*)spin)->children)
			//	nfspinbutton_set_child_position(spin);

			nfspinbutton_change_child_position(spin);
			break;

		case GDK_DELETE:
			{
				gchar *strTemp;
				guint cnt;
				guint i;

				if(spin->krepeat_src)
				{
					g_source_remove(spin->krepeat_src);
					spin->krepeat_src = 0;
				}

				if(spin->mrepeat_src)
				{
					g_source_remove(spin->mrepeat_src);
					spin->mrepeat_src = 0;
				}

				if(spin->bg_gc)
					g_object_unref(spin->bg_gc);

				if(spin->data)
				{	
					cnt = g_slist_length(spin->data);

					for(i=0; i<cnt; i++)
					{
						strTemp = (gchar*)g_slist_nth_data(spin->data, i);
						g_free(strTemp);
						strTemp = NULL;
					}

					g_slist_free(spin->data);
					spin->data = NULL;
				}

				if(spin->adjustment) {	 
					g_object_ref(spin->adjustment);
					gtk_object_sink(GTK_OBJECT(spin->adjustment));
				}

				// SKSHIN
				/*
				if(!((NFFIXED*)spin)->children) {
					if(spin->label) {
						nfui_nfobject_destroy(spin->label);
						spin->label = NULL; 
					}

					if(spin->up_btn) {
						nfui_nfobject_destroy(spin->up_btn);
						spin->up_btn = NULL; 
					}

					if(spin->down_btn) {
						nfui_nfobject_destroy(spin->down_btn);
						spin->down_btn = NULL; 
					}
				}
				*/
#if 0
				g_free(spin);
				spin = NULL;
				//#else
				memset(spin, 0, sizeof(NFSPINBUTTON));
				g_free(spin);
				spin = NULL;
#endif
			}
			break;

		case NFEVENT_CANCEL:
				if(spin->index != spin->pre_index) {
					if(spin->adjustment) {
						GtkAdjustment *adj = spin->adjustment;

						adj->value = adj->lower + (adj->step_increment * spin->pre_index);
					}

					spin->index = spin->pre_index;

					nfui_user_signal_emit((NFFIXED*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				}

				return FALSE;	
			break;


		case GDK_BUTTON_PRESS:
			//		case GDK_MOTION_NOTIFY:
	  	   	if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
	
			
			if(spin->krepeat_src)
			{
				g_source_remove(spin->krepeat_src);
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}
			break;


		case GDK_ENTER_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)spin))
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
			break;

		case GDK_LEAVE_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)spin))
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
		case GDK_BUTTON_RELEASE:
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			if(spin->krepeat_src)
			{
				g_source_remove(spin->krepeat_src);
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			if(spin->mrepeat_src || spin->krepeat_src)
			{
				if(spin->mrepeat_src)	g_source_remove(spin->mrepeat_src);
				if(spin->krepeat_src)	g_source_remove(spin->krepeat_src);
				spin->mrepeat_src = 0;
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}
			break;

		default:
			break;
	}

	return FALSE;
}


static gboolean nfspinbutton_label_cb(NFOBJECT *label, GdkEvent *event, gpointer data)
{
	NFSPINBUTTON *spin = NULL;

	switch(event->type)
	{
		case GDK_2BUTTON_PRESS:
			nfui_send_event(label->parent, event, FALSE);
		break;
		
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID dir;

			spin = (NFSPINBUTTON*)label->parent;

			if(spin->mrepeat_src || spin->krepeat_src)
			{
				if(spin->mrepeat_src)	g_source_remove(spin->mrepeat_src);
				if(spin->krepeat_src)	g_source_remove(spin->krepeat_src);
				spin->mrepeat_src = 0;
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}

			kevt = (GdkEventKey*)event;
			dir = (KEYPAD_KID)kevt->keyval;

			if(dir==KEYPAD_UP)
			{
				//				nfui_signal_emit(spin->up_btn, GDK_BUTTON_PRESS, FALSE);
				if(!spin->only_enterkey_action)
				{
					prvUpButtonProc(spin, TRUE);
				}
				else
				{
					prvUpButtonProc(spin, FALSE);                            
				}

				if(!spin->krepeat_src)
				{
					spin->rkey_id = KEYPAD_UP;
					spin->krepeat_src = g_timeout_add(SPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, spin);
				}
				return TRUE;
			}
			else if(dir==KEYPAD_DOWN)
			{
				//				nfui_signal_emit(spin->down_btn, GDK_BUTTON_PRESS, FALSE);
				if(!spin->only_enterkey_action)
				{
					prvDownButtonProc(spin, TRUE);
				}
				else
				{
					prvDownButtonProc(spin, FALSE);                            
				}

				if(!spin->krepeat_src)
				{
					spin->rkey_id = KEYPAD_DOWN;
					spin->krepeat_src = g_timeout_add(SPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, spin);
				}
				return TRUE;
			}
			else if(dir==KEYPAD_EXIT)
			{
				if(spin->index != spin->pre_index) {
					if(spin->adjustment) {
						GtkAdjustment *adj = spin->adjustment;

						adj->value = adj->lower + (adj->step_increment * spin->pre_index);
					}

					spin->index = spin->pre_index;

					nfui_user_signal_emit((NFFIXED*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				}

				return FALSE;	
			}
			else if(dir == KEYPAD_ENTER)
			{
				if(spin->index != spin->pre_index) {
					if(spin->adjustment) {
						GtkAdjustment *adj = spin->adjustment;

						adj->value = adj->lower + (adj->step_increment * spin->index);
					}

					spin->pre_index = spin->index;

					if(!spin->only_enterkey_action)
					{
						nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
					}
					else
					{
						nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_ENTERKEY_PRESS, FALSE);                   
					}
				}

				nfui_set_key_focus((NFOBJECT*)spin, FALSE);
				nfui_set_key_focus((NFOBJECT*)spin, TRUE);

				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);

				return TRUE;
			}
			else
			{
				return TRUE;
			}
		}
		break;

		case GDK_BUTTON_PRESS:
//		case GDK_MOTION_NOTIFY:			
			spin = (NFSPINBUTTON*)label->parent;
			if(spin->krepeat_src)
			{
				g_source_remove(spin->krepeat_src);
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}
			break;

		case GDK_LEAVE_NOTIFY:
		case GDK_BUTTON_RELEASE:
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			spin = (NFSPINBUTTON*)label->parent;

			if(spin->mrepeat_src)
			{
				g_source_remove(spin->mrepeat_src);
				spin->mrepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}
			if(spin->krepeat_src)
			{
				g_source_remove(spin->krepeat_src);
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}
			break;

		case GDK_SCROLL:
		{
			GdkEventScroll *sevt;

			sevt = (GdkEventScroll*)event;
			spin = (NFSPINBUTTON*)label->parent;

			if (spin->spin_fixed.object.kfocus == NFOBJECT_FOCUS)
			{
				if(sevt->direction == GDK_SCROLL_UP) {	
					prvUpButtonProc(spin, TRUE);
				}

				if(sevt->direction == GDK_SCROLL_DOWN) {
					prvDownButtonProc(spin, TRUE);
				}
			}
		}
		break;
		default:
		break;
	}

	return FALSE;
}

static gboolean nfspinbutton_press_cb(NFOBJECT *button, GdkEvent *event, gpointer data)
{
	NFSPINBUTTON *spin;
	
	switch(event->type) {
		case GDK_BUTTON_PRESS:			
			spin = (NFSPINBUTTON*)button->parent;

			if(spin->mrepeat_src || spin->krepeat_src)
			{
				if(spin->mrepeat_src)	g_source_remove(spin->mrepeat_src);
				if(spin->krepeat_src)	g_source_remove(spin->krepeat_src);
				spin->mrepeat_src = 0;
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}

	  	   	if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
	

			if(!spin->sensitive)
				return FALSE;

			if(spin->down_btn == button) {
				prvDownButtonProc(spin, TRUE);

				if(spin->pre_index != spin->index)
					spin->pre_index = spin->index;

				if(!spin->mrepeat_src)
				{
					spin->rkey_id = KEYPAD_DOWN;
					spin->mrepeat_src = g_timeout_add(SPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, spin);
				}

			}else if(spin->up_btn == button) {
				prvUpButtonProc(spin, TRUE);

				if(spin->pre_index != spin->index)
					spin->pre_index = spin->index;

				if(!spin->mrepeat_src)
				{
					spin->rkey_id = KEYPAD_UP;
					spin->mrepeat_src = g_timeout_add(SPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, spin);
				}
			}
			break;

		case GDK_LEAVE_NOTIFY:
		case GDK_BUTTON_RELEASE:
			spin = (NFSPINBUTTON*)button->parent;
			if(spin->mrepeat_src)
			{
				g_source_remove(spin->mrepeat_src);
				spin->mrepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}
			if(spin->krepeat_src)
			{
				g_source_remove(spin->krepeat_src);
				spin->krepeat_src = 0;

#ifdef __RELEASE_EVENT_ENABLE__
				nfui_user_signal_emit((NFOBJECT*)spin, NFEVENT_SPINBUTTON_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)spin, GDK_EXPOSE, TRUE);
#endif
			}
			break;

		default:
			break;
	}

	return FALSE;
}



NFOBJECT* nfui_spinbutton_new(gchar *data[], gint cnt, gint init) 
{
	NFSPINBUTTON *spin = NULL;
	gint i;

//	g_return_val_if_fail(data != NULL, NULL);
	g_return_val_if_fail(cnt >= 0, NULL);
	g_return_val_if_fail(init >= 0, NULL);

	spin = (NFSPINBUTTON*)imalloc(sizeof(NFSPINBUTTON));
	if(!spin) {	
		g_warning("NFSpinButton alloc error...");
		return NULL;
	}

#if 0
	spin->spin_fixed.object.parent = NULL;
	spin->spin_fixed.object.x = 0;
	spin->spin_fixed.object.y = 0;
	spin->spin_fixed.object.width = 140;
	spin->spin_fixed.object.height = 20;
	spin->spin_fixed.object.type = NFOBJECT_TYPE_NFSPINBUTTON;
	spin->spin_fixed.object.show = NFOBJECT_HIDE;
	spin->spin_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	spin->spin_fixed.object.kfocus = NFOBJECT_UNFOCUS;
	spin->spin_fixed.object.mfocus = NFOBJECT_UNFOCUS;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	spin->spin_fixed.object.pre_event_handler = NULL;
	spin->spin_fixed.object.default_event_handler = nfspinbutton_event_handler;
	spin->spin_fixed.object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)spin);

	spin->spin_fixed.object.width = 140;
	spin->spin_fixed.object.height = 20;
	spin->spin_fixed.object.type = NFOBJECT_TYPE_NFSPINBUTTON;
	spin->spin_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	spin->spin_fixed.object.default_event_handler = nfspinbutton_event_handler;
#endif

	spin->sensitive = TRUE;

	spin->label = (NFOBJECT*)nfui_nflabel_new("");
	nfui_nflabel_set_drawing_outline((NFLABEL*)(spin->label), FALSE);
	if(nftool_cur_language_is_eng())
	{
		nfui_nfobject_use_tooltip(spin->label, FALSE);
		nfui_nflabel_use_strip(spin->label, FALSE);
	}
	nfui_regi_post_event_callback(spin->label, (gpointer)nfspinbutton_label_cb);
	nfui_nfobject_show(spin->label); 

	spin->up_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(spin->up_btn, NFOBJECT_FOCUS_ON);
	nfui_regi_pre_event_callback(spin->up_btn, (gpointer)nfspinbutton_press_cb);
	nfui_nfobject_show(spin->up_btn); 

	spin->down_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(spin->down_btn, NFOBJECT_FOCUS_ON);
	nfui_regi_pre_event_callback(spin->down_btn, (gpointer)nfspinbutton_press_cb);
	nfui_nfobject_show(spin->down_btn); 
	spin->adjustment = NULL;

	// SKSHIN
	nfspinbutton_set_child_position(spin);

	for(i=0; i<cnt; i++) 
		spin->data = g_slist_append(spin->data, g_strdup(data[i]));

	spin->pre_index = spin->index = init;

	spin->align = NFALIGN_CENTER;
	spin->margin = 0;

	spin->spacing_type = NORMAL_SPACING;
       spin->only_enterkey_action = FALSE;

	spin->draw_outline = TRUE;
 	spin->skin_type = NFSPINBUTTON_TYPE_UNDEF;
  
	return (NFOBJECT*)spin;
}

NFOBJECT* nfui_spinbutton_new_index_with_range(gint init_index, gdouble min, gdouble max, gdouble step)
{
	NFSPINBUTTON *spin = NULL;
	gdouble temp;

//	g_return_val_if_fail(init >= 0, NULL);

	spin = (NFSPINBUTTON*)imalloc(sizeof(NFSPINBUTTON));
	if(!spin) {	
		g_warning("NFSpinButton alloc error...");
		return NULL;
	}

#if 0
	spin->spin_fixed.object.parent = NULL;
	spin->spin_fixed.object.x = 0;
	spin->spin_fixed.object.y = 0;
	spin->spin_fixed.object.width = 140;
	spin->spin_fixed.object.height = 20;
	spin->spin_fixed.object.type = NFOBJECT_TYPE_NFSPINBUTTON;
	spin->spin_fixed.object.show = NFOBJECT_HIDE;
	spin->spin_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	spin->spin_fixed.object.kfocus = NFOBJECT_UNFOCUS;
	spin->spin_fixed.object.mfocus = NFOBJECT_UNFOCUS;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	spin->spin_fixed.object.pre_event_handler = NULL;
	spin->spin_fixed.object.default_event_handler = nfspinbutton_event_handler;
	spin->spin_fixed.object.post_event_handler = NULL;
#else

	nfui_nfobject_init((NFOBJECT*)spin);

	spin->spin_fixed.object.width = 140;
	spin->spin_fixed.object.height = 20;
	spin->spin_fixed.object.type = NFOBJECT_TYPE_NFSPINBUTTON;
	spin->spin_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	spin->spin_fixed.object.default_event_handler = nfspinbutton_event_handler;
#endif

	spin->sensitive = TRUE;
	
	spin->label = (NFOBJECT*)nfui_nflabel_new("");
	nfui_nflabel_set_drawing_outline((NFLABEL*)(spin->label), FALSE);
	nfui_regi_post_event_callback(spin->label, (gpointer)nfspinbutton_label_cb);
	nfui_nfobject_show(spin->label); 

	spin->up_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(spin->up_btn, NFOBJECT_FOCUS_ON);
	nfui_regi_pre_event_callback(spin->up_btn, (gpointer)nfspinbutton_press_cb);
	nfui_nfobject_show(spin->up_btn); 

	spin->down_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(spin->down_btn, NFOBJECT_FOCUS_ON);
	nfui_regi_pre_event_callback(spin->down_btn, (gpointer)nfspinbutton_press_cb);
	nfui_nfobject_show(spin->down_btn); 

	temp = min + (init_index * step);
	spin->adjustment = (GtkAdjustment*)gtk_adjustment_new(temp, min, max, step, 10*step, 0.0);

	spin->data = NULL;
	spin->pre_index = spin->index = init_index;

    if (fabs(step) >= 1.0 || step == 0.0) spin->digits = 0;        
    else spin->digits = abs((gint)floor(log10(fabs(step))));

    if (spin->digits > 8) spin->digits = 8;

	spin->align = NFALIGN_CENTER;
	spin->margin = 0;

	spin->spacing_type = NORMAL_SPACING;
       spin->only_enterkey_action = FALSE;

	spin->draw_outline = TRUE;
	
	// SKSHIN
	nfspinbutton_set_child_position(spin);
       
	return (NFOBJECT*)spin;
}

NFOBJECT* nfui_spinbutton_new_value_with_range(gdouble init_value, gdouble min, gdouble max, gdouble step)
{
	NFSPINBUTTON *spin = NULL;
	gdouble temp;

//	g_return_val_if_fail(init >= 0, NULL);

	spin = (NFSPINBUTTON*)imalloc(sizeof(NFSPINBUTTON));
	if(!spin) {	
		g_warning("NFSpinButton alloc error...");
		return NULL;
	}

#if 0
	spin->spin_fixed.object.parent = NULL;
	spin->spin_fixed.object.x = 0;
	spin->spin_fixed.object.y = 0;
	spin->spin_fixed.object.width = 140;
	spin->spin_fixed.object.height = 20;
	spin->spin_fixed.object.type = NFOBJECT_TYPE_NFSPINBUTTON;
	spin->spin_fixed.object.show = NFOBJECT_HIDE;
	spin->spin_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	spin->spin_fixed.object.kfocus = NFOBJECT_UNFOCUS;
	spin->spin_fixed.object.mfocus = NFOBJECT_UNFOCUS;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	spin->spin_fixed.object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	spin->spin_fixed.object.pre_event_handler = NULL;
	spin->spin_fixed.object.default_event_handler = nfspinbutton_event_handler;
	spin->spin_fixed.object.post_event_handler = NULL;
#else

	nfui_nfobject_init((NFOBJECT*)spin);

	spin->spin_fixed.object.width = 140;
	spin->spin_fixed.object.height = 20;
	spin->spin_fixed.object.type = NFOBJECT_TYPE_NFSPINBUTTON;
	spin->spin_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	spin->spin_fixed.object.default_event_handler = nfspinbutton_event_handler;
#endif

	spin->sensitive = TRUE;
	
	spin->label = (NFOBJECT*)nfui_nflabel_new("");
	nfui_nflabel_set_drawing_outline((NFLABEL*)(spin->label), FALSE);
	nfui_regi_post_event_callback(spin->label, (gpointer)nfspinbutton_label_cb);
	nfui_nfobject_show(spin->label); 

	spin->up_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(spin->up_btn, NFOBJECT_FOCUS_ON);
	nfui_regi_pre_event_callback(spin->up_btn, (gpointer)nfspinbutton_press_cb);
	nfui_nfobject_show(spin->up_btn); 

	spin->down_btn = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(spin->down_btn, NFOBJECT_FOCUS_ON);
	nfui_regi_pre_event_callback(spin->down_btn, (gpointer)nfspinbutton_press_cb);
	nfui_nfobject_show(spin->down_btn); 

	spin->adjustment = (GtkAdjustment*)gtk_adjustment_new(init_value, min, max, step, 10*step, 0.0);

	spin->data = NULL;
	spin->pre_index = spin->index = (gint)((init_value-min)/step);

    if (fabs(step) >= 1.0 || step == 0.0) spin->digits = 0;        
    else spin->digits = abs((gint)floor(log10(fabs(step))));

    if (spin->digits > 8) spin->digits = 8;
    
	spin->align = NFALIGN_CENTER;
	spin->margin = 0;

	spin->spacing_type = NORMAL_SPACING;
       spin->only_enterkey_action = FALSE;

	spin->draw_outline = TRUE;
	
	// SKSHIN
	nfspinbutton_set_child_position(spin);
       
	return (NFOBJECT*)spin;
}

void nfui_spinbutton_set_skin_type(NFSPINBUTTON *spin, NFSPINBUTTON_TYPE type)
{
	g_return_val_if_fail(spin != NULL, NULL);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return NULL;
	}

	if (type == NFSPINBUTTON_TYPE_1)
		_set_spinbutton_type_1(spin);
	else if (type == NFSPINBUTTON_TYPE_POPUP_1)
		_set_spinbutton_type_popup_1(spin);
	else if (type == NFSPINBUTTON_TYPE_SUBTAB_1)
		_set_spinbutton_type_subtab_1(spin);		
	else if (type == NFSPINBUTTON_TYPE_POPUP_SMALL_1)
		_set_spinbutton_type_popup_small_1(spin);		
}

void nfui_spin_button_set_pango_font(NFSPINBUTTON *spin, gchar *pfont)
{
	g_return_if_fail(pfont);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return;
	}

	if (spin->skin_type == NFSPINBUTTON_TYPE_UNDEF) g_assert(0);

	if (spin->skin_type == NFSPINBUTTON_TYPE_1)
		nfui_nflabel_set_pango_font((NFLABEL*)(spin->label), pfont, COLOR_IDX(177));
	else if ((spin->skin_type == NFSPINBUTTON_TYPE_POPUP_1) || (spin->skin_type == NFSPINBUTTON_TYPE_POPUP_SMALL_1))
		nfui_nflabel_set_pango_font((NFLABEL*)(spin->label), pfont, COLOR_IDX(269));		
	else if (spin->skin_type == NFSPINBUTTON_TYPE_SUBTAB_1)
		nfui_nflabel_set_pango_font((NFLABEL*)(spin->label), pfont, COLOR_IDX(959));
}

gboolean nfui_spin_button_set_text(NFSPINBUTTON *spin, gchar* text)
{
	guint i;
	gchar *str;
	gdouble tmp;

	g_return_val_if_fail(spin != NULL, FALSE);
	g_return_val_if_fail(text != NULL, FALSE);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	if(!spin->bg_gc) {
		g_warning("can't set the text before drawing spin-button");
		return FALSE;
	}

	if(spin->adjustment) {
		tmp = g_strtod(text, NULL);
		
		if(tmp < spin->adjustment->lower ||
					spin->adjustment->upper < tmp) {
			g_warning("text value is out of range");

			return FALSE;
		} else {
			if(!((gint)tmp%(gint)spin->adjustment->step_increment)) {
				spin->adjustment->value = tmp; 
				spin->pre_index = spin->index = spin->adjustment->value/spin->adjustment->step_increment;
			}else {
				g_warning("text value is not correct");

				return FALSE;
			}
		}

		nfspinbutton_draw_bg(spin);

		if (nfui_nfobject_is_supported_multi_lang(spin))
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
		else
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

		nfui_nflabel_set_text((NFLABEL*)(spin->label), text);
		nfui_signal_emit(spin->label, GDK_EXPOSE, TRUE);

		return TRUE;
	}else {
		for(i=0; g_slist_nth(spin->data, i); i++) {
			str = (gchar*)g_slist_nth_data(spin->data, i);

			if(!strcmp(str, text)) {
				spin->pre_index = spin->index = i;

				nfspinbutton_draw_bg(spin);

				if (nfui_nfobject_is_supported_multi_lang(spin))
					nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
				else
					nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

				nfui_nflabel_set_text((NFLABEL*)(spin->label), text);
				nfui_signal_emit(spin->label, GDK_EXPOSE, TRUE);

				return TRUE;
			}
		}

		g_warning("%s is not in spin data", text);
	}

	return FALSE;
}


gboolean nfui_spin_button_set_text_no_expose(NFSPINBUTTON *spin, gchar* text)
{
	guint i;
	gchar *str;
	gdouble tmp;

	g_return_val_if_fail(spin != NULL, FALSE);
	g_return_val_if_fail(text != NULL, FALSE);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	if(spin->adjustment) {
		tmp = g_strtod(text, NULL);
		
		if(tmp < spin->adjustment->step_increment ||
					spin->adjustment->upper < tmp) {
			g_warning("text value is out of range");

			return FALSE;
		} else {
			if(!((gint)tmp%(gint)spin->adjustment->step_increment)) {
				spin->adjustment->value = tmp; 
				spin->pre_index = spin->index = spin->adjustment->value/spin->adjustment->step_increment;
			}else {
				g_warning("text value is not correct");

				return FALSE;
			}
		}

		return TRUE;
	}else {
		for(i=0; g_slist_nth(spin->data, i); i++) {
			str = (gchar*)g_slist_nth_data(spin->data, i);

			if(!strcmp(str, text)) {
				spin->pre_index = spin->index = i;

				return TRUE;
			}
		}

		g_warning("%s is not a spin data", text);
	}

	return FALSE;
}


gchar* nfui_spin_button_get_text(NFSPINBUTTON *spin)
{
	g_return_val_if_fail(spin != NULL, NULL);
	
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return NULL;
	}

	if(spin->adjustment)
		return nfspin_button_convert_gdouble_to_char(spin, GDOUBLE_TO_G_CHAR, spin->adjustment->value);	//warning :: memory leak....
	else 
		return (gchar*)g_slist_nth_data(spin->data, (guint)spin->index);
}


gint nfui_spin_button_get_value(NFSPINBUTTON *spin)
{
	g_return_val_if_fail(spin != NULL, 0);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return 0;
	}

	if(spin->adjustment)
		return (gint)spin->adjustment->value;
	
	g_error("only returns the numeric values");

	return 0;
}


gboolean 
nfui_spin_button_set_index(NFSPINBUTTON *spin, gint index)
{
	gchar *str = NULL;
	gdouble val;
	GtkAdjustment *adj;


	g_return_val_if_fail(spin != NULL, FALSE);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	if(spin->adjustment) {
		adj = spin->adjustment;

		val = adj->lower + (adj->step_increment * index);
		
		if(val < adj->lower || adj->upper < val) {
			g_warning("%s : index[%d], lower[%d], upper[%d]", __FUNCTION__, index, adj->lower, adj->upper);		
			g_warning("index is out of range");

			return FALSE;
		}


		str = nfspin_button_convert_gdouble_to_char(spin, GDOUBLE_TO_G_CHAR, val);
		adj->value = val;
		spin->pre_index = spin->index = index;

		nfspinbutton_draw_bg(spin);

		if (nfui_nfobject_is_supported_multi_lang(spin))
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
		else
			nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

		nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
		g_free(str);
		nfui_signal_emit(spin->label, GDK_EXPOSE, TRUE);

		return TRUE;
	}else {
		str = g_slist_nth_data(spin->data, index);
		if(str) {
			spin->pre_index = spin->index = index;

			nfspinbutton_draw_bg(spin);

			if (nfui_nfobject_is_supported_multi_lang(spin))
				nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
			else
				nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

			nfui_nflabel_set_text((NFLABEL*)(spin->label), str);
			nfui_signal_emit(spin->label, GDK_EXPOSE, TRUE);
		}else { 
			g_warning("%s:can't find the data. index[%d], spin->index[%d]", __FUNCTION__, index, spin->index);
			return FALSE;
		}
		return TRUE;
	}

	return FALSE;
}

gboolean 
nfui_spin_button_set_index_no_expose(NFSPINBUTTON *spin, gint index)
{
	gchar *str = NULL;
	gdouble val;
	GtkAdjustment *adj;


	g_return_val_if_fail(spin != NULL, FALSE);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	if(spin->adjustment) {
		adj = spin->adjustment;

		val = adj->lower + (adj->step_increment * index);
		
		if(val < adj->lower || adj->upper < val) {
			g_warning("%s : index[%d], lower[%d], upper[%d]", __FUNCTION__, index, adj->lower, adj->upper);		
			g_warning("index is out of range");
			return FALSE;
		}


		str = nfspin_button_convert_gdouble_to_char(spin, GDOUBLE_TO_G_CHAR, val);
		adj->value = val;
		spin->pre_index = spin->index = index;
		g_free(str);

		return TRUE;
	}else {
		str = g_slist_nth_data(spin->data, index);
		if(str) {
			spin->pre_index = spin->index = index;

		}else { 
			g_warning("%s:can't find the data. index[%d], spin->index[%d]", __FUNCTION__, index, spin->index);
			return FALSE;
		}
		return TRUE;
	}

	return FALSE;
}


gint nfui_spin_button_get_index(NFSPINBUTTON *spin)
{
	g_return_val_if_fail(spin != NULL, -1);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return -1;
	}

	return spin->index;
}


gboolean nfui_spin_button_set_data(NFSPINBUTTON *spin, gchar* data[], guint cnt)
{
	guint i;
	gchar *strTemp = NULL;
	guint cur_cnt;

	g_return_val_if_fail(spin != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	if(spin->data) {
		cur_cnt = g_slist_length(spin->data);

		for(i=0; i<cur_cnt; i++)
		{
			strTemp = (gchar*)g_slist_nth_data(spin->data, i);
			g_free(strTemp);
			strTemp = NULL;
		}

		g_slist_free(spin->data);
		spin->data = NULL;
	}

	for(i=0; i<cnt; i++)  {
		strTemp = g_strdup(data[i]);
		spin->data = g_slist_append(spin->data, strTemp);
	}
	spin->index = 0;

	strTemp = (gchar*)g_slist_nth_data(spin->data, (guint)spin->index);

	nfspinbutton_draw_bg(spin);

	if (nfui_nfobject_is_supported_multi_lang(spin))
		nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, TRUE);
	else
		nfui_nfobject_support_multi_lang((NFLABEL *)spin->label, FALSE);

	nfui_nflabel_set_text((NFLABEL*)(spin->label), strTemp);
	nfui_signal_emit(spin->label, GDK_EXPOSE, TRUE);

	return TRUE;
}


gboolean nfui_spin_button_set_data_no_expose(NFSPINBUTTON *spin, gchar* data[], guint cnt)
{
	guint i;
	gchar *strTemp = NULL;
	guint cur_cnt;

	g_return_val_if_fail(spin != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	if(spin->data) {
		cur_cnt = g_slist_length(spin->data);

		for(i=0; i<cur_cnt; i++)
		{
			strTemp = (gchar*)g_slist_nth_data(spin->data, i);
			g_free(strTemp);
			strTemp = NULL;
		}

		g_slist_free(spin->data);
		spin->data = NULL;
	}

	for(i=0; i<cnt; i++)  {
		strTemp = g_strdup(data[i]);
		spin->data = g_slist_append(spin->data, strTemp);
	}
	spin->index = 0;

	return TRUE;
}


void nfui_spin_button_set_align(NFSPINBUTTON *spin, nfalign_type align, guint margin)
{
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return;
	}

	spin->align = align;
	spin->margin = margin;

	nfui_nflabel_set_align((NFLABEL*)(spin->label), align, margin);
}


void nfui_spin_button_sensitive(NFSPINBUTTON *spin, gboolean sensitive)
{
	g_return_if_fail(spin != NULL); 
	
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return;
	}

	if(spin->sensitive != sensitive)	
		spin->sensitive = sensitive;

}

guint nfui_spin_button_remove_data(NFSPINBUTTON *spin, const gchar *data)
{
	guint cnt;
	guint i;
	gchar *strTemp;

	g_return_val_if_fail(spin != NULL, 0);
	g_return_val_if_fail(data != NULL, 0);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return 0;
	}

	cnt = g_slist_length(spin->data);

	for(i=0; i<cnt; i++) {
		strTemp = (gchar*)g_slist_nth_data(spin->data, i);

		if(!g_ascii_strcasecmp(strTemp, data)) {
			spin->data = g_slist_remove(spin->data, strTemp);
			g_free(strTemp);
			strTemp = NULL;

			if(g_slist_length(spin->data) == 0)			
				spin->index = -1;
			else
				spin->index = 0;

			return cnt-1;
		}
	}
	return cnt;
}


void nfui_spin_button_remove_all(NFSPINBUTTON *spin)
{
	guint cnt;
	guint i;
	gchar *strTemp;

	cnt = g_slist_length(spin->data);

	for(i=0; i<cnt; i++)
	{
		strTemp = (gchar*)g_slist_nth_data(spin->data, i);
		g_free(strTemp);
		strTemp = NULL;
	}

	g_slist_free(spin->data);
	spin->data = NULL;

	spin->index = -1;
}

gboolean nfui_spin_button_append_data(NFSPINBUTTON *spin, gchar *text)
{
	GtkWidget *menu_item;

	g_return_val_if_fail(spin != NULL, FALSE);
	g_return_val_if_fail(text != NULL, FALSE);

	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	spin->data = g_slist_append(spin->data, g_strdup(text));
	
	return TRUE;
}

gint nfui_spin_button_get_cnt(NFSPINBUTTON *spin)
{
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return 0;
	}

// added hmkong...
	if(spin->data) 
		return g_list_length(spin->data);

	return 0;
}

void nfui_spin_button_use_pango_cashing(NFSPINBUTTON *spin, gint cashing, gchar *key)
{
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return;
	}

	nfui_nflabel_use_pango_cashing((NFLABEL*)(spin->label), cashing, key);
}


void nfui_spin_button_set_spacing(NFSPINBUTTON *spin, nfutil_pango_spacing_type spacing_type)
{
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return;
	}

	spin->spacing_type = spacing_type;
}

void nfui_spin_button_set_only_enterkey_action(NFSPINBUTTON *spin, guint only_enterkey_action)
{
    	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return;
	}

	spin->only_enterkey_action = only_enterkey_action;
}


void nfui_spin_button_set_draw_outline(NFSPINBUTTON *spin, gboolean draw_outline)
{
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return;
	}

	spin->draw_outline = draw_outline;
}

NFOBJECT *nfui_spin_button_get_label(NFSPINBUTTON *spin)
{
	return spin->label;
}

gboolean nfui_spin_button_set_range(NFSPINBUTTON *spin, gdouble min, gdouble max)
{
	if(spin->spin_fixed.object.type != NFOBJECT_TYPE_NFSPINBUTTON)
	{
		return FALSE;
	}

	if(spin->adjustment) {	 
		gtk_adjustment_set_lower((GtkAdjustment*)spin->adjustment, min);
		gtk_adjustment_set_upper((GtkAdjustment*)spin->adjustment, max);
	}
	else
		return FALSE;
	return TRUE;
}

