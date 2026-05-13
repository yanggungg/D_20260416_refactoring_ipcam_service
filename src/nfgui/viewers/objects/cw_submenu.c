#include "support/nf_ui_image.h"
#include "support/event_loop.h"

#include "ix_mem.h"
#include "ix_func.h"

#include "nf_ui_tool.h"

#include "cw_submenu.h"
#include "vw_menu.h"

// [Private Method]

#define CW_SM_MV_LEVEL 2
#define STR_UNDEF 				""

static gint cw_sm_mv_level = 0;
static gchar *cw_sm_submenu_fname[SYS_SUBMENU_CNT] = { MK_IMG_SUBMENU_CAMERA,
											MK_IMG_SUBMENU_DISPLAY,
											MK_IMG_SUBMENU_SOUND,
											MK_IMG_SUBMENU_USER,
											MK_IMG_SUBMENU_NETWORK,
											MK_IMG_SUBMENU_SYSTEM,
											MK_IMG_SUBMENU_STORAGE,
											MK_IMG_SUBMENU_EVENT
											};

static gchar cw_sm_submenu_title[SYS_SUBMENU_CNT][30] = {	"CAMERA", "DISPLAY", "AUDIO", "USER", 
											"NETWORK", "SYSTEM", "STORAGE", "EVENT", 
											};

static gint	cw_sm_submenu_counts_of_item[SYS_SUBMENU_CNT];
static gchar *cw_sm_submenu_item[SYS_SUBMENU_CNT][MAX_TAB_CNT];


// [Private Member Method and function]
static gboolean cw_sm_event_handler(CWSUBMENU *cwwidget, GdkEvent *event, gpointer data);

static void cw_sm_paint(CWSUBMENU *cwwidget);

static void cw_sm_draw_bg(CWSUBMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable);
static void cw_sm_draw_icon(CWSUBMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable, guint x, guint y);


static void cw_sm_button_press (CWSUBMENU *cwwidget, gint x, gint y);
static void cw_sm_button_release (CWSUBMENU *cwwidget, gint x, gint y);
static void cw_sm_motion_notify (CWSUBMENU *cwwidget, gint x, gint y);


static void _init_submenu_text()
{
    cw_sm_submenu_counts_of_item[CAMERA_SUBMENU] = mcf.sys_sub1.cnt;
    vw_menu_get_str_sys_menu_sub1(cw_sm_submenu_item[CAMERA_SUBMENU]);

    cw_sm_submenu_counts_of_item[DISPLAY_SUBMENU] = mcf.sys_sub2.cnt;    
    vw_menu_get_str_sys_menu_sub2(cw_sm_submenu_item[DISPLAY_SUBMENU]);

    cw_sm_submenu_counts_of_item[AUDIO_SUBMENU] = mcf.sys_sub3.cnt;    
    vw_menu_get_str_sys_menu_sub3(cw_sm_submenu_item[AUDIO_SUBMENU]);

    cw_sm_submenu_counts_of_item[USER_SUBMENU] = mcf.sys_sub4.cnt;    
    vw_menu_get_str_sys_menu_sub4(cw_sm_submenu_item[USER_SUBMENU]);

    cw_sm_submenu_counts_of_item[NETWORK_SUBMENU] = mcf.sys_sub5.cnt;    
    vw_menu_get_str_sys_menu_sub5(cw_sm_submenu_item[NETWORK_SUBMENU]);

    cw_sm_submenu_counts_of_item[SYSTEM_SUBMENU] = mcf.sys_sub6.cnt;    
    vw_menu_get_str_sys_menu_sub6(cw_sm_submenu_item[SYSTEM_SUBMENU]);

    cw_sm_submenu_counts_of_item[STORAGE_SUBMENU] = mcf.sys_sub7.cnt;    
    vw_menu_get_str_sys_menu_sub7(cw_sm_submenu_item[STORAGE_SUBMENU]);

    cw_sm_submenu_counts_of_item[EVENT_SUBMENU] = mcf.sys_sub8.cnt;    
    vw_menu_get_str_sys_menu_sub8(cw_sm_submenu_item[EVENT_SUBMENU]);
}

static gboolean cw_sm_event_handler(CWSUBMENU *cwwidget, GdkEvent *event, gpointer data)
{
	//GdkDrawable *drawable;	
	//GdkGC *gc;

	switch(event->type)	
	{
		case GDK_EXPOSE : 
		cw_sm_paint(cwwidget); 
		break;

		case GDK_BUTTON_PRESS :	
		break;

		case GDK_BUTTON_RELEASE :
			cw_sm_button_release (cwwidget, event->button.x, event->button.y);
		break;

		case GDK_MOTION_NOTIFY :
			{
				cw_sm_mv_level++;
	
				if(cw_sm_mv_level >= CW_SM_MV_LEVEL)
				{
					cw_sm_motion_notify(cwwidget, event->button.x, event->button.y);
					cw_sm_mv_level = 0;
				}
			}
		break;	

		case GDK_DELETE :	
			// SKSHIN
			if (cwwidget->text_pixmap) g_object_unref(cwwidget->text_pixmap);
			cwwidget->text_pixmap = 0;
		break;

		default :
			break;
	}

	return FALSE;
	
}

static void 
cw_sm_paint(CWSUBMENU *cwwidget)
{
	GdkDrawable *drawable;
	cairo_t *cr;
	GdkGC *gc;
	GdkColor color;
	guint x, y;

	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);

	drawable = nfui_nfobject_get_window((NFOBJECT*)cwwidget);	
	gc = nfui_nfobject_get_gc((NFOBJECT*)cwwidget);

	// SKSHIN
	if(cwwidget->is_draw_bg)
	{
		cw_sm_draw_bg(cwwidget, gc, drawable);
			
		cwwidget->is_draw_bg = !cwwidget->is_draw_bg;

		///////////////////////////////////////////////////
		//GdkPixmap *text_pixmap;
		//gint text_pixmap_width;
		//gint text_pixmap_height;
		GdkColor ncolor;
		gint i;
		gint xpos = 0;
		gint ypos = 0;
		guint sel = cwwidget->sel_num;
		gint item_counts = cw_sm_submenu_counts_of_item[sel];
		
//		ifn_set_gdkcolor(110, 200, 255, &ncolor);
		if (!cwwidget->text_pixmap) {
			cwwidget->text_pixmap = gdk_pixmap_new (drawable,
													cwwidget->object.width, 
													item_counts * 46,
													-1);
		}
		
		for(i = 0 ; i < item_counts ; i++ )
		{
			gdk_draw_pixbuf(cwwidget->text_pixmap, gc, cwwidget->nofocusbar_pixbuf,
							0, 0, 0, ypos + (i * 46),
							cwwidget->nofocusbar_width, cwwidget->nofocusbar_height, 
							GDK_RGB_DITHER_NORMAL, 0, 0);
			
			nfutil_draw_text_with_pango(NULL, NULL, NULL, cwwidget->text_pixmap, gc, cw_sm_submenu_item[sel][i], 
							xpos, ypos + (i * 46), 260, 46, 
							nffont_get_pango_font(NFFONT_SMALL_SEMI_1), &UX_COLOR(cwwidget->subtext_normal_color), NFALIGN_CENTER, 0);
			
		}
									
		
		///////////////////////////////////////////////////
		
	}
	
	
	if(!cwwidget->is_draw_bg) {
		cw_sm_draw_bg(cwwidget, gc, drawable);
		cw_sm_draw_icon(cwwidget, gc, drawable, x, y);
		//cw_mm_draw_submenu(cwwidget, gc, drawable, pixmap);
		//cw_mm_draw_submenu_items(cwwidget, gc, drawable, pixmap);
	}

	// SKSHIN	
	nfui_nfobject_gc_unref(gc);
}

static void
cw_sm_draw_bg(CWSUBMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable)
{
	GdkPixmap *pixmap = NULL;
	GdkColor fcolor;
	guint sel = cwwidget->sel_num;
	gint bg_width;
	gint bg_height;
	nffont_type font_idx;

	pixmap = gdk_pixmap_new(drawable,
			cwwidget->object.width,
			cwwidget->object.height,
			-1);

	gdk_draw_pixbuf(pixmap, gc, cwwidget->bg_pixbuf, 
							0, 0, 
							0, 0, 
							cwwidget->bg_width, cwwidget->bg_height, 
							GDK_RGB_DITHER_NORMAL, 0, 0);


//	ifn_set_gdkcolor(255, 255, 255, &fcolor);

	if(nftool_cur_language_is_japanese())
		font_idx = NFFONT_MEDIUM_NORMAL;
	else
		font_idx = NFFONT_XLARGE_NORMAL_1;	
	
	nfutil_draw_text_with_pango(NULL, NULL, NULL, pixmap, gc,
								cw_sm_submenu_title[sel], 
								0, cwwidget->maintext_y, 
								260, 32,
								nffont_get_pango_font(font_idx), &UX_COLOR(cwwidget->maintext_color), NFALIGN_CENTER, 0);

	gdk_draw_drawable (drawable, gc, pixmap, 0, 0,
			cwwidget->object.x, cwwidget->object.y,
			cwwidget->w_width, cwwidget->w_height);

	g_object_unref(pixmap);
}

static void 
cw_sm_draw_icon(CWSUBMENU *cwwidget, GdkGC *gc, GdkDrawable *drawable, guint x, guint y)
{
	guint sel = cwwidget->sel_num;

	GdkColor ocolor;
//	ifn_set_gdkcolor(58, 33, 0, &ocolor);
	
	gint item_counts = cw_sm_submenu_counts_of_item[sel];

	gint i;
	gint xpos = 0;
	gint ypos = 271;

	gdk_draw_drawable(drawable, gc, cwwidget->text_pixmap, 0, 0,
					  0, ypos,
					  cwwidget->w_width, item_counts * 46);
	
	for(i = 0 ; i < item_counts ; i++ )
	{
		if(cwwidget->sel_subitem == i)
		{

			gdk_draw_pixbuf(drawable, gc, cwwidget->focusbar_pixbuf,
							0, 0, 
							xpos + 6, ypos + (i * 46),
							cwwidget->focusbar_width, cwwidget->focusbar_height, 
							GDK_RGB_DITHER_NORMAL, 0, 0);

			nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, 
									cw_sm_submenu_item[sel][i], 
									xpos, ypos + (i * 46), 260, 46, 
									nffont_get_pango_font(NFFONT_SMALL_NORMAL_1), &UX_COLOR(cwwidget->subtext_focus_color), NFALIGN_CENTER, 0);
		}
		
	}
	
}
 

static void 
cw_sm_button_release (CWSUBMENU *cwwidget, gint x, gint y)
{
	gint start_y;

	start_y = cwwidget->maintext_y + 70;

	if(y > cwwidget->maintext_y)
		nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_SETUP_SUBMENU_CHANGED_RELEASE, FALSE);

}

static void cw_sm_motion_notify (CWSUBMENU *cwwidget, gint x, gint y)
{
	guint sel = cwwidget->sel_num;
	gint item_counts = cw_sm_submenu_counts_of_item[sel];
	
	gint start_y;
	start_y = cwwidget->maintext_y + 70;
	
	int i;

	gint current_submenu = 0;	

	
	if( y > cwwidget->maintext_y )
	{
		for(i = 0 ; i < item_counts ; i++)
		{
			if( y >= start_y && y <= (cwwidget->maintext_y + start_y) )
			{
				current_submenu = i;
			}

			start_y += 46;
		}

		if( current_submenu != cwwidget->sel_subitem )
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y;


			cwwidget->sel_subitem = current_submenu;

			nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
			drawable = nfui_nfobject_get_window((NFOBJECT*)cwwidget);	
			gc = nfui_nfobject_get_gc((NFOBJECT*)cwwidget);

			cw_sm_draw_icon(cwwidget, gc, drawable, x, y);

			nfui_nfobject_gc_unref(gc);
		}
		
	}

}


// [Public Member Method and function]


CWSUBMENU *cw_sm_new(guint sel_num, guint xpos, guint ypos)
{
	CWSUBMENU *cwwidget;

	cwwidget = (CWSUBMENU*)imalloc(sizeof(CWSUBMENU));
	
	nfui_nfobject_init((NFOBJECT*)cwwidget);

	cwwidget->sel_num = sel_num;
	cwwidget->w_width = xpos;  
	cwwidget->w_height = ypos; 

	cwwidget->is_draw_bg = TRUE;
	
	
	cwwidget->object.width = xpos;
	cwwidget->object.height = ypos;
	cwwidget->object.type = NFOBJECT_TYPE_CWSUBMENU;
	cwwidget->object.use_focus = NFOBJECT_FOCUS_ON;
	cwwidget->object.default_event_handler = cw_sm_event_handler;
	
	cwwidget->spacing_type = NORMAL_SPACING;

	cwwidget->maintext_y = 200;

	cwwidget->maintext_color = COLOR_IDX(700);
	cwwidget->subtext_normal_color = COLOR_IDX(704);
	cwwidget->subtext_focus_color = COLOR_IDX(705);

	cwwidget->sel_subitem = 0;
	cwwidget->focusbar_pixbuf 	= nfui_get_image_from_file(IMG_SUBMENU_FOCUS_BAR, NULL);
	cwwidget->focusbar_width 	= gdk_pixbuf_get_width(cwwidget->focusbar_pixbuf);
	cwwidget->focusbar_height 	= gdk_pixbuf_get_height(cwwidget->focusbar_pixbuf);

	cwwidget->nofocusbar_pixbuf 	= nfui_get_image_from_file(IMG_SUBMENU_NOFOCUS_BAR, NULL);
	cwwidget->nofocusbar_width 		= gdk_pixbuf_get_width(cwwidget->nofocusbar_pixbuf);
	cwwidget->nofocusbar_height 	= gdk_pixbuf_get_height(cwwidget->nofocusbar_pixbuf);

	cwwidget->bg_pixbuf = nfui_get_image_from_file(cw_sm_submenu_fname[sel_num], NULL);
	cwwidget->bg_width 	= gdk_pixbuf_get_width(cwwidget->bg_pixbuf);
	cwwidget->bg_height = gdk_pixbuf_get_height(cwwidget->bg_pixbuf);

    _init_submenu_text();

	return cwwidget;
	
}

void cw_sm_set_focusbar(CWSUBMENU *cwwidget, gchar *path)
{
	cwwidget->focusbar_pixbuf 	= nfui_get_image_from_file(path, NULL);
	cwwidget->focusbar_width 	= gdk_pixbuf_get_width(cwwidget->focusbar_pixbuf);
	cwwidget->focusbar_height 	= gdk_pixbuf_get_height(cwwidget->focusbar_pixbuf);
}

void cw_sm_set_nofocusbar(CWSUBMENU *cwwidget, gchar *path)
{
	cwwidget->nofocusbar_pixbuf 	= nfui_get_image_from_file(path, NULL);
	cwwidget->nofocusbar_width 		= gdk_pixbuf_get_width(cwwidget->nofocusbar_pixbuf);
	cwwidget->nofocusbar_height 	= gdk_pixbuf_get_height(cwwidget->nofocusbar_pixbuf);
}

void cw_sm_set_ypos_maintext(CWSUBMENU *cwwidget, gint pos)
{
	cwwidget->maintext_y = pos;
}

void cw_sm_set_color_maintext(CWSUBMENU *cwwidget, guint color_idx)
{
	cwwidget->maintext_color = color_idx;
}

void cw_sm_set_color_subtext_normal(CWSUBMENU *cwwidget, guint color_idx)
{
	cwwidget->subtext_normal_color = color_idx;
}

void cw_sm_set_color_subtext_focus(CWSUBMENU *cwwidget, guint color_idx)
{
	cwwidget->subtext_focus_color = color_idx;
}

void cw_sm_unref_pixbuf(CWSUBMENU *cwwidget)
{
}

void cw_sm_destroy(CWSUBMENU *cwwidget)
{
}

void 
cw_sm_update(CWSUBMENU *cwwidget)
{
	cw_sm_paint(cwwidget);	
}

gint cw_sm_get_selected_subitem(CWSUBMENU *cwwidget)
{
	return cwwidget->sel_subitem;
}

void cw_sm_redraw_bg(CWSUBMENU *cwwidget)
{
	cwwidget->is_draw_bg = TRUE;
}

void cw_sm_sel_submenu_init(CWSUBMENU *cwwidget)
{
	cwwidget->sel_subitem = 0;
}

void 
cw_sm_set_remote_select(CWSUBMENU *cwwidget, gint current_menu)
{
	cwwidget->sel_subitem = current_menu;
	cw_sm_paint(cwwidget);	
}

gint 
cw_sm_get_menu_counts(CWSUBMENU *cwwidget, gint current_menu)
{
	return cw_sm_submenu_counts_of_item[current_menu] - 1;
}

