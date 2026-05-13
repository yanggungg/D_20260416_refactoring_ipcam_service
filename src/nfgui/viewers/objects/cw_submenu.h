#ifndef __CW_SUBMENU_H__
#define __CW_SUBMENU_H__

#include "nfobject.h"
#include "../../support/util.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
//#include "cw_util.h"

G_BEGIN_DECLS

#define CWSM(cwwidget) ((CWSUBMENU*)cwwidget)

#define CW_SM_WIDTH 267

typedef struct _CWSUBMENU CWSUBMENU;

struct _CWSUBMENU {
	NFOBJECT object;
	nfutil_pango_spacing_type spacing_type;

	guint sel_num;
	gshort w_width;
	gshort w_height;

	gboolean is_draw_bg;
	//GdkPixbuf *submenu_pixbuf;
	//gint submenu_width;
	//gint submenu_height;

	// sub items
	gint sel_subitem;

	gint maintext_y;

	guint maintext_color;
	guint subtext_normal_color;
	guint subtext_focus_color;	

	GdkPixbuf *bg_pixbuf;
	gint *bg_width;
	gint *bg_height;

	GdkPixbuf *focusbar_pixbuf;
	gint focusbar_width;
	gint focusbar_height;

	GdkPixbuf *nofocusbar_pixbuf;
	gint nofocusbar_width;
	gint nofocusbar_height;

	GdkPixmap *text_pixmap;
};

CWSUBMENU *cw_sm_new(guint sel_num, guint xpos, guint ypos);

void cw_sm_set_ypos_maintext(CWSUBMENU *cwwidget, gint pos);
void cw_sm_set_focusbar(CWSUBMENU *cwwidget, gchar *path);
void cw_sm_set_nofocusbar(CWSUBMENU *cwwidget, gchar *path);
void cw_sm_set_color_maintext(CWSUBMENU *cwwidget, guint color_idx);
void cw_sm_set_color_subtext_normal(CWSUBMENU *cwwidget, guint color_idx);
void cw_sm_set_color_subtext_focus(CWSUBMENU *cwwidget, guint color_idx);
void cw_sm_unref_pixbuf(CWSUBMENU *cwwidget);
void cw_sm_destroy(CWSUBMENU *cwwidget);
void cw_sm_update(CWSUBMENU *cwwidget);
gint cw_sm_get_selected_subitem(CWSUBMENU *cwwidget);

void cw_sm_redraw_bg(CWSUBMENU *cwwidget);
void cw_sm_sel_submenu_init(CWSUBMENU *cwwidget);

void cw_sm_set_remote_select(CWSUBMENU *cwwidget, gint current_menu);

gint cw_sm_get_menu_counts(CWSUBMENU *cwwidget, gint current_menu);
	
G_END_DECLS

#endif








