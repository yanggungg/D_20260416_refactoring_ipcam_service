#ifndef __CW_MAINMENU_H__
#define __CW_MAINMENU_H__

#include "nfobject.h"
#include "../../support/util.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include "support/nf_ui_image.h"


#define CW_MM_MV_LEVEL 1

G_BEGIN_DECLS

#define CWMM(cwwidget) ((CWMAINMENU*)cwwidget)

//#define CW_MM_WIDTH 1531
#define CW_MM_WIDTH 1649
#define CW_MM_HEIGHT 211

typedef struct _CWMAINMENU CWMAINMENU;

struct _CWMAINMENU {
	// Public method
	NFOBJECT object;
	nfutil_pango_spacing_type spacing_type;

	guint icon_x_pos[8];
	guint icon_y_pos;
	
	guint menu_color;

	GdkPixmap *pixmap;
	
	GdkPixbuf *bg_pixbuf;
	gint bg_width, bg_height;
	gboolean is_redraw_bg_pixbuf;
	
	GdkPixbuf *nicon_pixbuf[8];
	gint nicon_width[8];
	gint nicon_height[8];

	GdkPixbuf *noicon_pixbuf[8];
	gint noicon_width[8];
	gint noicon_height[8];

	gint sel_noicon;
	gint sel_submenu;
	gint sel_subitem;
	gboolean is_sel_complete;	
	gboolean is_draw_segment;	
};

CWMAINMENU *cw_mm_new();

void cw_mm_unref_pixbuf(CWMAINMENU *cwwidget);
void cw_mm_destroy(CWMAINMENU *cwwidget);

void cw_mm_set_color_menu_text(CWMAINMENU *cwwidget, guint color_idx);

gint cw_mm_get_topmenu_number(CWMAINMENU *cwwidget);
//int cw_mm_get_submenu_number(CWMAINMENU *cwwidget);

//gboolean cw_mm_is_complete(CWMAINMENU *cwwidget);

void cw_mm_redraw_bg(CWMAINMENU *cwwidget);
void cw_mm_set_draw_segment(CWMAINMENU *cwwidget, gint segment_number);
void cw_mm_update(CWMAINMENU *cwwidget);
void cw_mm_set_sel_icon(CWMAINMENU *cwwidget, gint sel_noicon);

G_END_DECLS

#endif




