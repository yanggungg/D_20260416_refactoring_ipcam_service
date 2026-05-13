#include "support/event_loop.h"
#include "cw_slider.h"
#include <math.h>
#include "ix_mem.h"
#include "ix_func.h"
#include "nf_ui_tool.h"

#define MOUSE_LEFT_BUTTON						(1)
#define MOUSE_RIGHT_BUTTON						(3)

// [Private Method]

#define CW_SLIDER_MV_LEVEL 7

static gint cw_slider_mv_level = 0;


// [Private Member Method and function]
static gboolean cw_slider_event_handler(CWSLIDER *cwwidget, GdkEvent *event, gpointer data);

static void cw_slider_paint(CWSLIDER *cwwidget);

static void cw_slider_draw(CWSLIDER *cwwidget, GdkGC *gc, GdkDrawable *drawable, GdkPixmap *pixmap);

static void cw_slider_button_press (CWSLIDER *cwwidget, gint x, gint y);
static void cw_slider_button_release (CWSLIDER *cwwidget, gint x, gint y);
static void cw_slider_motion_notify (CWSLIDER *cwwidget, gint x, gint y);

static gboolean cw_slider_event_handler(CWSLIDER *cwwidget, GdkEvent *event, gpointer data)
{
	switch(event->type)	
	{
		case GDK_EXPOSE : 
		{
			cwwidget->is_draging = FALSE;
			cw_slider_paint(cwwidget); 
		}
		break;
		
		case GDK_BUTTON_PRESS :	
		{
            GdkEventButton *bevent;
            guint x, y;
            
			bevent = (GdkEventButton *)event;
			
            if(bevent->button == MOUSE_RIGHT_BUTTON) return FALSE;
		
			cwwidget->is_draging = TRUE;

			nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
			cw_slider_motion_notify(cwwidget, event->button.x-x, event->button.y-y);			
		}
		break;

		case GDK_BUTTON_RELEASE :
		{
			guint x, y;
			nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
			cw_slider_button_release (cwwidget, event->button.x+x, event->button.y+y);

			cwwidget->is_draging = FALSE;
			cwwidget->slider_kind = 0;
			cw_slider_paint(cwwidget);
		}
		break;
		
		case GDK_MOTION_NOTIFY :
			{
			GdkEventMotion *mevt;
					guint x, y;

			mevt = (GdkEventMotion*)event;
	
			if ((cwwidget->is_draging == TRUE) && (mevt->state & GDK_BUTTON1_MASK))
			{
					nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
					cw_slider_motion_notify(cwwidget, event->button.x-x, event->button.y-y);
				}
				else
				{
					cw_slider_mv_level++;

					if(cw_slider_mv_level >= CW_SLIDER_MV_LEVEL)
					{
						cwwidget->slider_kind = 1;
						cw_slider_paint(cwwidget);
						cw_slider_mv_level = 0;
					}
				}
			}
		break;	

		case GDK_LEAVE_NOTIFY :
		{
			if (cwwidget->is_draging)
				nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_CWSLIDER_CHANGED_RELEASE, FALSE);
		
			cwwidget->slider_kind = 0;
			cw_slider_paint(cwwidget);
		}
		break;
		
		case GDK_DELETE :	
		break;
		
		default :
			break;
	}

	return FALSE;
	
}

static void 
cw_slider_paint(CWSLIDER *cwwidget)
{
	GdkDrawable *drawable;
	cairo_t *cr;
	GdkGC *gc;
	GdkColor color;
	guint x, y;

	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);

	drawable = nfui_nfobject_get_window((NFOBJECT*)cwwidget);	
	gc = nfui_nfobject_get_gc((NFOBJECT*)cwwidget);

	//color = cw_util_set_dec_gdkcolor(255, 0, 0);
	//gdk_gc_set_rgb_fg_color(gc, &color);	
	//gdk_draw_rectangle (drawable, gc, FALSE, x, y, cwwidget->w_width, cwwidget->w_height);

	GdkPixbuf *pixmap = NULL;

	pixmap = gdk_pixmap_new(drawable,
	                        cwwidget->object.width, cwwidget->object.height,
	                        -1);

	cw_slider_draw(cwwidget, gc, drawable, pixmap);
			
	gdk_draw_drawable (drawable, gc, pixmap, 0, 0,
						x, y,
						cwwidget->w_width, cwwidget->w_height);

	g_object_unref(pixmap);
	//g_object_unref(gc);
	
}

static void
cw_slider_draw(CWSLIDER *cwwidget, GdkGC *gc, GdkDrawable *drawable, GdkPixmap *pixmap)
{
	guint x, y;
	gint bg_col_idx;

	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);

//	ifn_set_gdkcolor(cwwidget->bg_color_r, cwwidget->bg_color_g, cwwidget->bg_color_b, &color);
	
	bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)cwwidget, NFOBJECT_STATE_NORMAL);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
	gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0, cwwidget->w_width, cwwidget->w_height);


	if(nfui_nfobject_is_disabled((NFOBJECT*)cwwidget))
	{
		gdk_draw_pixbuf(pixmap, gc, cwwidget->bg_d_pixbuf, 
								0, 0, 
								cwwidget->slider_d_width/2, 0, 
								cwwidget->bg_d_width, cwwidget->bg_d_height, 
								GDK_RGB_DITHER_NORMAL, 0, 0);
	}
	else
	{
		gdk_draw_pixbuf(pixmap, gc, cwwidget->bg_pixbuf, 
								0, 0, 
								cwwidget->slider_n_width/2, 0, 
								cwwidget->bg_width, cwwidget->bg_height, 
								GDK_RGB_DITHER_NORMAL, 0, 0);
	}

	gdouble per = (gdouble)cwwidget->bg_width / (gdouble)(cwwidget->max-cwwidget->min);
	gint focus_x = (cwwidget->slider_value-cwwidget->min) * per;

	if(focus_x > cwwidget->bg_width) focus_x = cwwidget->bg_width;

	if(nfui_nfobject_is_disabled((NFOBJECT*)cwwidget))
	{
		gdk_draw_pixbuf(pixmap, gc, cwwidget->slider_d_pixbuf, 0, 0,
									focus_x, 
									0,
									cwwidget->slider_d_width, 
									cwwidget->slider_d_height,
									GDK_RGB_DITHER_NORMAL, 0, 0);
	}
	else if(cwwidget->slider_kind == 0)
	{
		gdk_draw_pixbuf(pixmap, gc, cwwidget->slider_n_pixbuf, 0, 0,
									focus_x, 
									0,
									cwwidget->slider_n_width, 
									cwwidget->slider_n_height,
									GDK_RGB_DITHER_NORMAL, 0, 0);
	}
	else if(cwwidget->slider_kind == 1)
	{
		gdk_draw_pixbuf(pixmap, gc, cwwidget->slider_o_pixbuf, 0, 0,
									focus_x, 
									0,
									cwwidget->slider_o_width, 
									cwwidget->slider_o_height,
									GDK_RGB_DITHER_NORMAL, 0, 0);
	}
	else if(cwwidget->slider_kind == 2)
	{
		gdk_draw_pixbuf(pixmap, gc, cwwidget->slider_s_pixbuf, 0, 0,
									focus_x, 
									0,
									cwwidget->slider_s_width, 
									cwwidget->slider_s_height,
									GDK_RGB_DITHER_NORMAL, 0, 0);
	}
	
		
}

static void cw_slider_motion_notify (CWSLIDER *cwwidget, gint x, gint y)
{
	gdouble focus_x_tmp = (gdouble)cwwidget->w_width / (gdouble)cwwidget->counts;
	guint x1, y1;
    gint value;
	
	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x1, &y1);

	gint slider_half = cwwidget->slider_n_width / 2;

	if( x >= slider_half && x <= (cwwidget->w_width - slider_half) && 
		y >= 0 && y <= cwwidget->w_height)
	{
		gdouble width = cwwidget->w_width - cwwidget->slider_n_width;
		gdouble per = cwwidget->counts;

		gdouble xpos = (x - slider_half);
	
		value = cwwidget->min + (xpos / width * per);

		if (value == cwwidget->max+1)  value--;

		if (cwwidget->slider_value != value)
		{
		    cwwidget->slider_value = value;
    		cwwidget->slider_kind = 2;
    		cw_slider_paint(cwwidget);
    		nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_CWSLIDER_CHANGED_RELEASE, FALSE);
		}
	}
	
}



static void 
cw_slider_button_release (CWSLIDER *cwwidget, gint x, gint y)
{
}

// [Public Member Method and function]

CWSLIDER *cw_slider_new(gint slider_value, guint width, guint height)
{
	CWSLIDER *cwwidget;

	cwwidget = (CWSLIDER*)imalloc(sizeof(CWSLIDER));
	
	nfui_nfobject_init((NFOBJECT*)cwwidget);

	cwwidget->slider_value = slider_value;
	cwwidget->w_width = width;  
	cwwidget->w_height = height; 
			
	cwwidget->object.width = width;
	cwwidget->object.height = height;
	cwwidget->object.type = NFOBJECT_TYPE_CWSLIDER;
	cwwidget->object.use_focus = NFOBJECT_FOCUS_ON;
	cwwidget->object.default_event_handler = cw_slider_event_handler;
	
	cwwidget->slider_n_pixbuf 	= nfui_get_image_from_file((IMG_SLIDEBAR_N), NULL);
	cwwidget->slider_n_width 	= gdk_pixbuf_get_width(cwwidget->slider_n_pixbuf);
	cwwidget->slider_n_height	= gdk_pixbuf_get_height(cwwidget->slider_n_pixbuf);

	cwwidget->slider_o_pixbuf 	= nfui_get_image_from_file((IMG_SLIDEBAR_O), NULL);
	cwwidget->slider_o_width 	= gdk_pixbuf_get_width(cwwidget->slider_o_pixbuf);
	cwwidget->slider_o_height	= gdk_pixbuf_get_height(cwwidget->slider_o_pixbuf);

	cwwidget->slider_s_pixbuf 	= nfui_get_image_from_file((IMG_SLIDEBAR_S), NULL);
	cwwidget->slider_s_width 	= gdk_pixbuf_get_width(cwwidget->slider_s_pixbuf);
	cwwidget->slider_s_height	= gdk_pixbuf_get_height(cwwidget->slider_s_pixbuf);

	cwwidget->slider_d_pixbuf 	= nfui_get_image_from_file((IMG_SLIDEBAR_D), NULL);
	cwwidget->slider_d_width 	= gdk_pixbuf_get_width(cwwidget->slider_d_pixbuf);
	cwwidget->slider_d_height	= gdk_pixbuf_get_height(cwwidget->slider_d_pixbuf);

	cwwidget->bg_pixbuf = nftool_create_slider_n_image(width-cwwidget->slider_n_width);
	cwwidget->bg_width 	= gdk_pixbuf_get_width(cwwidget->bg_pixbuf);
	cwwidget->bg_height = gdk_pixbuf_get_height(cwwidget->bg_pixbuf);

	cwwidget->bg_d_pixbuf = nftool_create_slider_d_image(width-cwwidget->slider_d_width);
	cwwidget->bg_d_width  = gdk_pixbuf_get_width(cwwidget->bg_d_pixbuf);
	cwwidget->bg_d_height = gdk_pixbuf_get_height(cwwidget->bg_d_pixbuf);

	cwwidget->spacing_type = NORMAL_SPACING;
	cwwidget->slider_kind = 0;
	cwwidget->is_draging = FALSE; 

	return cwwidget;
	
}

CWSLIDER *cw_slider_small_new(gint slider_value, guint width, guint height)
{
	CWSLIDER *cwwidget;

	cwwidget = (CWSLIDER*)imalloc(sizeof(CWSLIDER));
	
	nfui_nfobject_init((NFOBJECT*)cwwidget);

	cwwidget->slider_value = slider_value;
	cwwidget->w_width = width;  
	cwwidget->w_height = height; 
			
	cwwidget->object.width = width;
	cwwidget->object.height = height;
	cwwidget->object.type = NFOBJECT_TYPE_CWSLIDER;
	cwwidget->object.use_focus = NFOBJECT_FOCUS_ON;
	cwwidget->object.default_event_handler = cw_slider_event_handler;
	
	cwwidget->slider_n_pixbuf 	= nfui_get_image_from_file((MK_IMG_SLIDEBAR_SMALL_N), NULL);
	cwwidget->slider_n_width 	= gdk_pixbuf_get_width(cwwidget->slider_n_pixbuf);
	cwwidget->slider_n_height	= gdk_pixbuf_get_height(cwwidget->slider_n_pixbuf);

	cwwidget->slider_o_pixbuf 	= nfui_get_image_from_file((MK_IMG_SLIDEBAR_SMALL_O), NULL);
	cwwidget->slider_o_width 	= gdk_pixbuf_get_width(cwwidget->slider_o_pixbuf);
	cwwidget->slider_o_height	= gdk_pixbuf_get_height(cwwidget->slider_o_pixbuf);

	cwwidget->slider_s_pixbuf 	= nfui_get_image_from_file((MK_IMG_SLIDEBAR_SMALL_S), NULL);
	cwwidget->slider_s_width 	= gdk_pixbuf_get_width(cwwidget->slider_s_pixbuf);
	cwwidget->slider_s_height	= gdk_pixbuf_get_height(cwwidget->slider_s_pixbuf);

	cwwidget->slider_d_pixbuf 	= nfui_get_image_from_file((MK_IMG_SLIDEBAR_SMALL_D), NULL);
	cwwidget->slider_d_width 	= gdk_pixbuf_get_width(cwwidget->slider_d_pixbuf);
	cwwidget->slider_d_height	= gdk_pixbuf_get_height(cwwidget->slider_d_pixbuf);

	cwwidget->bg_pixbuf = nftool_create_slider_small_n_image(width-cwwidget->slider_n_width);
	cwwidget->bg_width 	= gdk_pixbuf_get_width(cwwidget->bg_pixbuf);
	cwwidget->bg_height = gdk_pixbuf_get_height(cwwidget->bg_pixbuf);

	cwwidget->bg_d_pixbuf = nftool_create_slider_small_d_image(width-cwwidget->slider_d_width);
	cwwidget->bg_d_width  = gdk_pixbuf_get_width(cwwidget->bg_d_pixbuf);
	cwwidget->bg_d_height = gdk_pixbuf_get_height(cwwidget->bg_d_pixbuf);

	cwwidget->spacing_type = NORMAL_SPACING;
	cwwidget->slider_kind = 0;
	cwwidget->is_draging = FALSE; 

	return cwwidget;
	
}

void 
cw_slider_update(CWSLIDER *cwwidget)
{
	cw_slider_paint(cwwidget);	
}

void 
cw_slider_set_range(CWSLIDER *cwwidget, gint min, gint max, gint counts)
{
	cwwidget->min = min;
	cwwidget->max = max;
	cwwidget->counts = counts;
}

gint cw_slider_get_value(CWSLIDER *cwwidget)
{
	return cwwidget->slider_value;
}

gint cw_slider_set_value(CWSLIDER *cwwidget, gint slider_value)
{
    if ((slider_value < cwwidget->min) || (slider_value > cwwidget->max))
    {
		g_warning("%s : index[%d], min[%d], max[%d]", __FUNCTION__, slider_value, cwwidget->min, cwwidget->max);
        g_warning("%s, %d, value is out of range", __FUNCTION__, __LINE__);
        return -1;
    }

	cwwidget->slider_value = slider_value;
	return 0;
}


void cw_slider_set_bg_color(CWSLIDER *cwwidget, gint r, gint g, gint b)
{
//	cwwidget->bg_color_r = r;
//	cwwidget->bg_color_g = g;
//	cwwidget->bg_color_b = b;
}

