#include "support/event_loop.h"
#include "cw_mainmenu.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "nf_ui_tool.h"

// [Private Method]

static int cw_mm_mv_level;

static gchar	*cw_mm_nicon_fname[8] = {  
											IMG_SUBMENU_N_CAMERA,
											IMG_SUBMENU_N_DISPLAY,
											IMG_SUBMENU_N_SOUND,
											IMG_SUBMENU_N_USER,
											IMG_SUBMENU_N_NETWORK,
											IMG_SUBMENU_N_SYSTEM,
											IMG_SUBMENU_N_STORAGE,
											IMG_SUBMENU_N_EVENT
										};

static gchar	*cw_mm_noicon_fname[8] = { 	
											IMG_SUBMENU_O_CAMERA,
											IMG_SUBMENU_O_DISPLAY,
											IMG_SUBMENU_O_SOUND,
											IMG_SUBMENU_O_USER,
											IMG_SUBMENU_O_NETWORK,
											IMG_SUBMENU_O_SYSTEM,
											IMG_SUBMENU_O_STORAGE,
											IMG_SUBMENU_O_EVENT
										};



// [Private Member Method and function]

static gboolean cw_mm_event_handler(CWMAINMENU *cwwidget, GdkEvent *event, gpointer data);

static void cw_mm_paint(CWMAINMENU *cwwidget);
static void cw_mm_draw_bg(CWMAINMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable, GdkPixmap *pixmap);
static void cw_mm_draw_icons(CWMAINMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable, guint x, guint y);

static void cw_mm_button_press (CWMAINMENU *cwwidget, gint x, gint y);
static void cw_mm_button_release (CWMAINMENU *cwwidget, gint x, gint y);
static void cw_mm_motion_notify (CWMAINMENU *cwwidget, gint x, gint y);

static gboolean 
cw_mm_event_handler(CWMAINMENU *cwwidget, GdkEvent *event, gpointer data)
{

	switch(event->type)	
	{
		case GDK_EXPOSE : cw_mm_paint(cwwidget); break;
		case GDK_ENTER_NOTIFY :	break;
		case GDK_LEAVE_NOTIFY : break;
		case GDK_2BUTTON_PRESS : break;
		case GDK_BUTTON_PRESS :	
			//cw_mm_button_press (cwwidget, event->button.x , event->button.y);
		break;
		case GDK_BUTTON_RELEASE :
			cw_mm_button_release (cwwidget, event->button.x , event->button.y);
			
		break;
		case GDK_MOTION_NOTIFY :
			{
				cw_mm_mv_level++;
	
				if(cw_mm_mv_level >= CW_MM_MV_LEVEL){
					cw_mm_motion_notify (cwwidget, event->button.x , event->button.y);
					cw_mm_mv_level = 0;
				}
			}
		break;	

		case GDK_DELETE :	
			cw_mm_unref_pixbuf(cwwidget);
			break;

		default :
			break;
	}

	return FALSE;
	
}

static void 
cw_mm_paint(CWMAINMENU *cwwidget)
{

	GdkDrawable *drawable;
	cairo_t *cr;
	GdkGC *gc;
	guint x, y;
	gint i;
	
	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);

	drawable = nfui_nfobject_get_window((NFOBJECT*)cwwidget);	
	gc = nfui_nfobject_get_gc((NFOBJECT*)cwwidget);

	if( cwwidget->is_redraw_bg_pixbuf == TRUE )
	{
		//GdkPixmap *pixmap = NULL;
		//GdkPixbuf *pixmap = NULL;

		cwwidget->pixmap = gdk_pixmap_new(drawable,
	                           cwwidget->object.width,
	                            cwwidget->object.height,
	                            -1);

		cw_mm_draw_bg(cwwidget, gc, drawable, cwwidget->pixmap);

		/////////////////////////////////////////////////////////////////
		
		for( i = 0 ; i < 8 ; i++ )
		{	
			gdk_draw_pixbuf(cwwidget->pixmap, gc, cwwidget->nicon_pixbuf[i],
							0, 0, 
							cwwidget->icon_x_pos[i], 
							cwwidget->icon_y_pos, 
							cwwidget->nicon_width[i], cwwidget->nicon_height[i], 
							GDK_RGB_DITHER_NORMAL, 0, 0);
		}

		gdk_draw_drawable ( drawable, gc, cwwidget->pixmap, 0, 0,
							0, 0,
							cwwidget->object.width, cwwidget->object.height);		
 
		cwwidget->is_redraw_bg_pixbuf = FALSE;
	}
	else
	{
		if(cwwidget->is_draw_segment == TRUE)
		{
			gdk_draw_drawable ( drawable, gc, cwwidget->pixmap, 0, 0,
								0, 0,
								cwwidget->object.width, cwwidget->object.height);	
			
			cwwidget->is_draw_segment = !cwwidget->is_draw_segment;

			cwwidget->sel_noicon = cwwidget->sel_submenu;
		}
		else
		{
			cw_mm_draw_icons(cwwidget, gc, drawable, x, y);
		}
	}	
	
	// SKSHIN	
	nfui_nfobject_gc_unref(gc);
}

static void
cw_mm_draw_bg(CWMAINMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable, GdkPixmap *pixmap)
{
		
	gint i;
	GdkColor fcolor;
	gint cw_mm_label_end_ypos = 158;
	gchar cw_mm_label_name[8][30] = {"CAMERA", "DISPLAY", "AUDIO", "USER", 
									 "NETWORK", "SYSTEM", "STORAGE", "EVENT"};
	nffont_type font_idx;


	if(nftool_cur_language_is_japanese())
		font_idx = NFFONT_MEDIUM_NORMAL;
	else
		font_idx = NFFONT_XLARGE_NORMAL;	

	
	gdk_draw_pixbuf(pixmap, gc, cwwidget->bg_pixbuf, 
							0, 0, 
							0, 
							0, 
							cwwidget->bg_width, cwwidget->bg_height, 
							GDK_RGB_DITHER_NORMAL, 0, 0);


	for( i = 0 ; i < 8 ; i++ )
	{	
		nfutil_draw_text_with_pango(NULL, NULL, NULL, pixmap, gc, cw_mm_label_name[i],
									cwwidget->icon_x_pos[i], cw_mm_label_end_ypos, 
									cwwidget->nicon_width[i], 26, 
									nffont_get_pango_font(font_idx), &UX_COLOR(cwwidget->menu_color), NFALIGN_CENTER, 0);
	}
	
}

static void 
cw_mm_draw_icons(CWMAINMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable, guint x, guint y)
{
	gint i; 
	
	for( i = 0 ; i < 8 ; i++ )
	{	
		
			if(cwwidget->sel_noicon == i)
			{
				gdk_draw_pixbuf(drawable, gc, cwwidget->noicon_pixbuf[i],
								0, 0, 
								cwwidget->icon_x_pos[i], 
								cwwidget->icon_y_pos, 
								cwwidget->noicon_width[i], cwwidget->noicon_height[i], 
								GDK_RGB_DITHER_NORMAL, 0, 0);
			}
			else
			{
				gdk_draw_pixbuf(drawable, gc, cwwidget->nicon_pixbuf[i],
								0, 0, 
								cwwidget->icon_x_pos[i], 
								cwwidget->icon_y_pos, 
								cwwidget->nicon_width[i], cwwidget->nicon_height[i], 
								GDK_RGB_DITHER_NORMAL, 0, 0);
			}
		
	}
}

static void 
cw_mm_button_release (CWMAINMENU *cwwidget, gint x, gint y)
{

	if(cwwidget->sel_submenu != cwwidget->sel_noicon)
	{
		cwwidget->sel_submenu = cwwidget->sel_noicon;
		nfui_user_signal_emit((NFOBJECT*)cwwidget, 
							NFEVENT_SETUP_MAINMENU_CHANGED_RELEASE, FALSE);
	}

}

static void 
cw_mm_motion_notify (CWMAINMENU *cwwidget, gint x, gint y)
{
	gint xpos = cwwidget->icon_x_pos[0];
	gint ypos = 8;
	gint icon_w = cwwidget->noicon_width[0];
	gint icon_h = cwwidget->noicon_height[0];
	gint i;

	for(i = 0 ; i < 8 ; i++)
	{
		
		if(x > xpos && x < (cwwidget->icon_x_pos[i] + icon_w) && y > ypos && y < (ypos+icon_h+158) )
		{
			if(cwwidget->sel_noicon != i)
			{
				cwwidget->sel_noicon = i;
				
				cw_mm_paint(cwwidget);
			}
					
			break;
		}

		xpos = cwwidget->icon_x_pos[i];
	}
	
}

 
// [Public Member Method and function]

CWMAINMENU *cw_mm_new()
{
	CWMAINMENU *cwwidget;

	cwwidget = (CWMAINMENU*)imalloc(sizeof(CWMAINMENU));
	
	nfui_nfobject_init((NFOBJECT*)cwwidget);

	cwwidget->object.type = NFOBJECT_TYPE_CWMAINMENU;	
	cwwidget->object.use_focus = NFOBJECT_FOCUS_ON;
	cwwidget->object.default_event_handler = cw_mm_event_handler;

	cwwidget->spacing_type = NORMAL_SPACING;

	cwwidget->bg_pixbuf = nfui_get_image_from_file(IMG_MAINMENU_BG, NULL);
	cwwidget->bg_width =  gdk_pixbuf_get_width(cwwidget->bg_pixbuf);
	cwwidget->bg_height = gdk_pixbuf_get_height(cwwidget->bg_pixbuf);

	cwwidget->is_redraw_bg_pixbuf = TRUE;

	gint i;

	for( i = 0 ; i < 8 ; i++ )
	{
		cwwidget->nicon_pixbuf[i] = nfui_get_image_from_file(cw_mm_nicon_fname[i], NULL);
		cwwidget->nicon_width[i] = gdk_pixbuf_get_width(cwwidget->nicon_pixbuf[i]);
		cwwidget->nicon_height[i] = gdk_pixbuf_get_height(cwwidget->nicon_pixbuf[i]);

		cwwidget->noicon_pixbuf[i] = nfui_get_image_from_file(cw_mm_noicon_fname[i], NULL);
		cwwidget->noicon_width[i] = gdk_pixbuf_get_width(cwwidget->noicon_pixbuf[i]);
		cwwidget->noicon_height[i] = gdk_pixbuf_get_height(cwwidget->noicon_pixbuf[i]);
	}
	
	cwwidget->sel_noicon = 0;
	cwwidget->menu_color = COLOR_IDX(702);

	return cwwidget;
	
}

void cw_mm_set_color_menu_text(CWMAINMENU *cwwidget, guint color_idx)
{
	cwwidget->menu_color = color_idx;
}

void cw_mm_icon_xpos(CWMAINMENU *cwwidget, guint xpos[])
{
	gint i;

	for (i = 0; i < 8; i++)
	{
		cwwidget->icon_x_pos[i] = xpos[i];
	}
}

void cw_mm_icon_ypos(CWMAINMENU *cwwidget, guint ypos)
{
	cwwidget->icon_y_pos = 8;
}

void cw_mm_unref_pixbuf(CWMAINMENU *cwwidget)
{
	gint i;
	
	if (cwwidget->pixmap) g_object_unref(cwwidget->pixmap);
	cwwidget->pixmap = 0;
}

void cw_mm_destroy(CWMAINMENU *cwwidget)
{
}

gint 
cw_mm_get_topmenu_number(CWMAINMENU *cwwidget)
{
	return cwwidget->sel_submenu;
}

void cw_mm_redraw_bg(CWMAINMENU *cwwidget)
{
	cwwidget->is_redraw_bg_pixbuf = TRUE;
}

void cw_mm_set_draw_segment(CWMAINMENU *cwwidget, gint segment_number)
{
	cwwidget->is_draw_segment = TRUE;
}

void 
cw_mm_update(CWMAINMENU *cwwidget)
{
	cw_mm_paint(cwwidget);	
}

void cw_mm_set_sel_icon(CWMAINMENU *cwwidget, gint sel_noicon)
{
	cwwidget->sel_noicon = sel_noicon;
	//cw_mm_paint(cwwidget); 
}


